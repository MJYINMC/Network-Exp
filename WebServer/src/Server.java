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
        // �̳߳أ����б�ԭ����thread����ߵĲ�������
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
                    // �����̣߳������ڵ�������
                    threadPool.execute(new Client(clientSocket, id, pass, routes));
                    System.out.println(String.format("[Remote]%s ���ӵ�Web Server!", clientSocket.getRemoteSocketAddress()));
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        });
        // �����ڳ����쳣�˳�ʱ�ᱻ���ã��类ctrl c��������������������
        Runtime.getRuntime().addShutdownHook(
            new Thread(
                ()->{
                    System.out.println("���������жϣ�");
                    try{
                        socket.close();
                    }catch(IOException e){
                        e.printStackTrace();
                    }
                    System.out.println("����socket�ɹ��رգ����ٽ��������ӣ�");
                    try {
                        // �̳߳ؾܽ������ύ������ͬʱ�ȴ��̳߳��������ִ����Ϻ�ر��̳߳�
                        threadPool.shutdown();
                        // ÿ��һ�������̳߳�״̬�����̳߳��Ѿ�û�л�߳�ʱ������true
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
        // ��ȡ�����ļ��Ӷ�����
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
    // IO���
    private Socket socket;
    private BufferedReader in;
    private OutputStream out;
    // login �� pass �ǵ�¼��������, routes�洢������·��������·����ӳ��
    private String login;
    private String pass;
    private Properties routes;
    // ��ʽ����������� (���캯���в�����������Զ�������С)
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
                // readLine������һ�����з�CRLF�ͻ�ֹͣ
                String line = in.readLine();
                if(line == null){
                    // ���Զ����������Ͽ��Զ�socketʱ��readLine�᷵��һ��null��string
                    error = true;
                    break;
                }
                String[] request_info = line.split(" ");
                handleRequest(request_info[0], request_info[1]);
                // ��������finally��䣬����socket����
                break;
            }
        }catch(IOException e) {
            System.out.println("TCP���ӳ����쳣");
            error = true;
        }finally{
            if(error){
                System.out.println(String.format("[Remote]%s �ĻỰ���ִ���!", socket.getRemoteSocketAddress()));
            }
            System.out.println(String.format("[Remote]%s �ķ��������", socket.getRemoteSocketAddress()));
            // ����ʱ��Ϊ��������Ϊ������ȷ��ɣ�����Ҫ�ر�socket���ͷ���Դ
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
        // ���ʲ����ڵ�ҳ��ʱ����null
        filepath = routes.getProperty(path);
        code = filepath == null ? 404 : 200;
        if(filepath == null){
            filepath = routes.getProperty("/404");
        }
        // ��ȡ��Դ�ļ���File����
        File resource = new File(filepath);
        String filename = resource.getName();
        // ��ȡ�ļ���չ��
        String extension = filename.substring(filename.lastIndexOf(".") + 1);
        // ���ò�ͬ��type�ֶ�
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
        // ��ʱ��Ҫ��һ������http�����е��ֶ�
        char[] buf = new char[65535];
        int length = 0;
        try {
            length = in.read(buf);
        } catch (IOException e) {
            e.printStackTrace();
        }
        String request = new String(buf, 0, length);
        
        // ʹ��������ʽ�ж��û�����
        String pattern = "login=(\\S*)&pass=(\\S*)";
        Pattern r = Pattern.compile(pattern);
        Matcher m = r.matcher(request);
        String username = "";
        String password = "";
        String reply = "<html><body>Permission denied</body></html>";
        if(m.find()){
            // m.group(0)������request
            username = m.group(1);
            password = m.group(2);
        }
        
        // �����û����У��
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
            // ��ջ�����
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