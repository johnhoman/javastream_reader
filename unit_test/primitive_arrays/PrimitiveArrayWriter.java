import java.util.HashMap;
import java.util.Map.Entry;
import java.io.FileOutputStream;
import java.io.ObjectOutputStream;
import java.lang.Math;
import java.io.IOException;

class PrimitiveArrayWriter {
    
    public static void main(String[] args) throws IOException {
        /* int types */
        int int_max = 2147483647;
        int int_min = -2147483648;
        int []array_unsigned = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        int []array_signed = {0, -1, -2, -3, -4, -5, -6, -7, -8, -9};
        int []array_limits = {int_max, 0, int_min};
        
        /*
        try 
            (FileOutputStream file = new FileOutputStream("int_array_limits.ser");
            ObjectOutputStream out = new ObjectOutputStream(file); ){
            out.writeObject(array_limits);
        }
        catch (Exception e){
            e.printStackTrace();
        }*/
        
        float []float_array_signed = {-3.45f, -34.55f, -23.3f};
        float []float_array_unsigned = {3.45f, 34.55f, 23.3f};
        float []float_array_limits = {0.0f, (float)Math.pow(3.4, 38), -(float)Math.pow(3.4, 38)};
        /*
        try (FileOutputStream file = new FileOutputStream("float_array_limits.ser");
             ObjectOutputStream out = new ObjectOutputStream(file); ){
             out.writeObject(float_array_limits);
        }
        catch (Exception e){
            e.printStackTrace();
        } */

        FileOutputStream fos;
        ObjectOutputStream oos;

        double []double_array_signed = {-0.2343134, -1234132.3431, -312431.3, 0, -1.1234};
        fos = new FileOutputStream("double_array_signed.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(double_array_signed);
        fos.close();
        oos.close();

        double []double_array_unsigned = {0.2343134, 1234132.3431, 312431.3, 0, 1.1234};
        fos = new FileOutputStream("double_array_unsigned.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(double_array_unsigned);
        fos.close();
        oos.close();

        double []double_array_limits = {Double.MIN_VALUE, 0, Double.MAX_VALUE, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY};
        fos = new FileOutputStream("double_array_limits.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(double_array_limits);
        fos.close();
        oos.close();       
        
        double []double_array_empty = {};
        fos = new FileOutputStream("double_array_empty.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(double_array_empty);
        fos.close();
        oos.close();

        double []double_array_new = new double[300];
        fos = new FileOutputStream("double_array_new.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(double_array_new);
        System.out.println(double_array_new.length);
        fos.close();
        oos.close();

        byte []byte_array_signed = {};
        byte []byte_array_unsigned = {};
        byte []byte_array_limits = {Byte.MAX_VALUE, 0, Byte.MAX_VALUE};
        byte []byte_array_empty = {};
        byte []byte_array_new = new byte[500];

        char []char_array_signed = {};
        char []char_array_unsigned = {};
        char []char_array_limits = {Character.MAX_VALUE, 0, Character.MIN_VALUE};

        long []long_array_signed = {-97609L, -906987806L, -79855734652564357L, -987809L};
        long []long_array_unsigned = {-97609L, -906987806L, -79855734652564357L, -987809L};
        long []long_array_limits = {Long.MAX_VALUE, Long.MIN_VALUE, 0};

        fos = new FileOutputStream("long_array_signed.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(long_array_signed);
        fos.close();
        oos.close();
        
        fos = new FileOutputStream("long_array_unsigned.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(long_array_unsigned);
        fos.close();
        oos.close();

        fos = new FileOutputStream("long_array_limits.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(long_array_limits);
        fos.close();
        oos.close();
        
        

        short []short_array_signed = {-(short)12321, -(short)123, -(short)22222, -(short)1232, (short)0};
        short []short_array_unsigned = {(short)0, (short)121, (short)2323, (short)3343, (short)3234, (short)2123, (short)234};
        short []short_array_limits = {Short.MAX_VALUE, 0, Short.MIN_VALUE};
        short []short_array_new = new short[10000];
        short []short_array_empty = {};

        fos = new FileOutputStream("short_array_signed.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(short_array_signed);
        fos.close();
        oos.close();

        fos = new FileOutputStream("short_array_unsigned.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(short_array_unsigned);
        fos.close();
        oos.close();

        fos = new FileOutputStream("short_array_limits.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(short_array_limits);
        fos.close();
        oos.close();

        fos = new FileOutputStream("short_array_new.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(short_array_new);
        fos.close();
        oos.close();

        fos = new FileOutputStream("short_array_empty.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(short_array_empty);
        fos.close();
        oos.close();


        boolean []boolean_array_true = {true, true, true, true, true, true};
        boolean []boolean_array_false = {false};
        boolean []boolean_array_mixed = {true, false, true, true, false};

        fos = new FileOutputStream("boolean_array_true.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(boolean_array_true);
        fos.close();
        oos.close();

        fos = new FileOutputStream("boolean_array_false.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(boolean_array_false);
        fos.close();
        oos.close();

        fos = new FileOutputStream("boolean_array_mixed.ser");
        oos = new ObjectOutputStream(fos);
        oos.writeObject(boolean_array_mixed);
        fos.close();
        oos.close();


    }

}