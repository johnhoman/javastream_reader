import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

class WrapperPrimitiveTypes {
    public static void main(String []args) throws IOException{
        Double a = new Double(10);
        FileOutputStream fos = new FileOutputStream("double_wrapper.ser");
        ObjectOutputStream oos = new ObjectOutputStream(fos);
        oos.writeObject(a);
        oos.close();
        fos.close();
    }
}