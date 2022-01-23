import java.io.*;
import java.net.*;
import java.util.concurrent.*;
import java.util.regex.*;
import java.lang.Runtime;
import java.util.Properties;

public class Server{
    private String id;
    private String pass;
    private int port;
    private ExecutorService threadPool;
    private ServerSocket socket;
    private Properties routes;
    public Server(String id){
        this.id = id;
        this.pass =  id.substring(id.length() - 4, id.length());
        this.port =  Integer.parseInt(pass) ;
        // 线程池，具有比原生的thread类更高的并发能力
        this.threadPool = Executors.newCachedThreadPool();
        try {
            Properties props = new Properties();
            BufferedReader bufferedReader = new BufferedReader(new FileReader("routes.properties"));
            props.load(bufferedReader);
            this.routes = props;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void start(){
        this.threadPool.execute( ()->{
            try{
                socket = new ServerSocket(port);
                System.out.println(String.format("Listening on %s:%s", socket.getInetAddress().toString(), socket.getLocalPort()));
                while (true){
                    Socket clientSocket = socket.accept();
                    // 开启线程，服务于单个请求
                    threadPool.execute(new Client(clientSocket, id, pass, routes));
                    System.out.println(String.format("[Remote]%s 连接到Web Server!", clientSocket.getRemoteSocketAddress()));
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        });
        // 整个在程序异常退出时会被调用（如被ctrl c），可以用来做清理工作
        Runtime.getRuntime().addShutdownHook(
            new Thread(
                ()->{
                    System.out.println("服务器被中断！");
                    try{
                        socket.close();
                    }catch(IOException e){
                        e.printStackTrace();
                    }
                    System.out.println("监听socket成功关闭！不再接受新连接！");
                    try {
                        // 线程池拒接收新提交的任务，同时等待线程池里的任务执行完毕后关闭线程池
                        threadPool.shutdown();
                        // 每隔一毫秒检测线程池状态，当线程池已经没有活动线程时，返回true
                        while(threadPool.awaitTermination(1, TimeUnit.MILLISECONDS) == false){
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            )
        );
    }
    
    public static void main(String[] args) {
        // 读取配置文件从而启动
        try{
            Properties props = new Properties();
            BufferedReader bufferedReader = new BufferedReader(new FileReader("config.properties"));
            props.load(bufferedReader);
            Server myserver = new Server(props.getProperty("id"));
            myserver.start();
        }catch(IOException e){
            e.printStackTrace();
        }
    }
}

class Client implements Runnable{
    // IO相关
    private Socket socket;
    private BufferedReader in;
    private OutputStream out;
    // login 和 pass 是登录名和密码, routes存储了请求路径到物理路径的映射
    private String login;
    private String pass;
    private Properties routes;
    // 流式的输出缓冲区 (构造函数中不传入参数则自动增长大小)
    private ByteArrayOutputStream buf;

    boolean error = false;
    Client(Socket s, String login, String pass, Properties routes){
        this.socket = s;
        this.login = login;
        this.pass = pass;
        this.routes = routes;
        this.buf = new ByteArrayOutputStream();
        try {
            in = new BufferedReader(
                                new InputStreamReader(
                                    s.getInputStream()));
            out = s.getOutputStream();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void run(){
        try {
            while(true){
                // readLine读到第一个换行符CRLF就会停止
                String line = in.readLine();
                if(line == null){
                    // 当对端主机主动断开对端socket时，readLine会返回一条null的string
                    error = true;
                    break;
                }
                String[] request_info = line.split(" ");
                handleRequest(request_info[0], request_info[1]);
                // 完成请求后到finally语句，结束socket连接
                break;
            }
        }catch(IOException e) {
            System.out.println("TCP连接出现异常");
            error = true;
        }finally{
            if(error){
                System.out.println(String.format("[Remote]%s 的会话出现错误!", socket.getRemoteSocketAddress()));
            }
            System.out.println(String.format("[Remote]%s 的服务结束！", socket.getRemoteSocketAddress()));
            // 无论时因为错误还是因为请求正确完成，都需要关闭socket，释放资源
            try{
                socket.close();
            }catch(IOException e){
                e.printStackTrace();
            }
        }
    }

    public void handleRequest(String method, String path){
        switch(method){
            case "GET":
                handleGET(path);
                break;
            case "POST":
                handlePOST(path);
                break;
        }
    }

    public void handleGET(String path){
        int code;
        String filepath;
        // 访问不存在的页面时读出null
        filepath = routes.getProperty(path);
        code = filepath == null ? 404 : 200;
        if(filepath == null){
            filepath = routes.getProperty("/404");
        }
        // 获取资源文件的File对象
        File resource = new File(filepath);
        String filename = resource.getName();
        // 获取文件拓展名
        String extension = filename.substring(filename.lastIndexOf(".") + 1);
        // 设置不同的type字段
        String type;
        switch (extension) {
            case "txt":
                type = "text/plain";
                break;            
            case "jpg":
                type = "image/jpeg";
                break;
            case "html":
                type = "text/html";
                break;            
            default:
                type = "text/html";
                break;
        }
        writeHeader(code, resource.length(), type);
        writeBody(resource);
        sendResponse();
    }

    public void handlePOST(String path){
        if(!path.equals("/dopost")){
            File resource = new File(routes.getProperty("/404"));
            writeHeader(404, resource.length(), "text/html");
            writeBody(resource);
            sendResponse();
            return;
        }
        // 此时需要进一步检验http请求中的字段
        char[] buf = new char[65535];
        int length = 0;
        try {
            length = in.read(buf);
        } catch (IOException e) {
            e.printStackTrace();
        }
        String request = new String(buf, 0, length);
        
        // 使用正则表达式判断用户输入
        String pattern = "login=(\\S*)&pass=(\\S*)";
        Pattern r = Pattern.compile(pattern);
        Matcher m = r.matcher(request);
        String username = "";
        String password = "";
        String reply = "<html><body>Permission denied</body></html>";
        if(m.find()){
            // m.group(0)是整个request
            username = m.group(1);
            password = m.group(2);
        }
        
        // 进行用户身份校验
        if(username.equals(login) && password.equals(pass))
            reply = "<html><body>Login success</body></html>";
        writeHeader(200, reply.length(), "text/html");
        writeBody(reply);
        sendResponse();
    }

    public void writeBody(String body){
        try{
            buf.write(body.getBytes("UTF-8"));
            buf.flush();
        }catch(IOException e){
            e.printStackTrace();
        }
    }

    public void writeBody(File resource){
        try{
            FileInputStream fstream = new FileInputStream(resource);
            buf.write(fstream.readAllBytes());
            fstream.close();
            buf.flush();
        }catch(IOException e){
            e.printStackTrace();
        }
    }    

    public void writeHeader(int code, long length, String type){
        byte[] res_code;
        try{
            // 清空缓冲区
            buf.reset();
            switch(code){
                case 200:
                    res_code = "HTTP/1.1 200 OK\r\n".getBytes("UTF-8");
                    break;
                case 404:
                default:
                    res_code = "HTTP/1.1 404 Not Found\r\n".getBytes("UTF-8");
            }
            buf.write(res_code);
            buf.write(("Server: " + socket.getLocalAddress() + "\n").getBytes("UTF-8"));
            buf.write(("Content-Type: " + type + ";charset=UTF-8\n").getBytes("UTF-8"));
            buf.write(("Content-Length: " + length + "\n\n").getBytes("UTF-8"));
        }catch(IOException e) {
            e.printStackTrace();
        }
    }

    public void sendResponse(){
        try {
            out.write(buf.toByteArray());
            out.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}