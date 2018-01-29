"""

"""

import unittest
from numpy import allclose
from math import pow
from jso_reader import _test_parse_primitive_array

class TestParsePrimitiveArray(unittest.TestCase):

    """This class will test several files will a single 
    primitive type array saved to it. 

    primitive types
    ---------------
        'B': byte {1 bytes - signed 2s complimenet}
        'C': char {2 bytes - }
        'D': double
        'F': float
        'I': int
        'J': long
        'S': short
        'Z': boolean
    """
    # 4 byte int arrays
    def test_parse_int_array_all_positive(self):
        """test a list of int is being parsed from the stream 
        correctly
        """
        filename = "primitive_arrays/int_array_unsigned.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

        self.assertEqual(from_file, expected)

    def test_parse_int_array_all_negative(self):
        # array with negatives
        filename = "primitive_arrays/int_array_signed.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [0, -1, -2, -3, -4, -5, -6, -7, -8, -9]
        
        self.assertEqual(from_file, expected)

    def test_parse_int_array_at_limits(self):
        # array at the limits of an integer
        filename = "primitive_arrays/int_array_limits.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [2147483647, 0, -2147483648]

        self.assertEqual(from_file, expected)

    # 4 bytes floating point arrays
    def test_parse_prim_float_array_all_positive(self):
        filename = 'primitive_arrays/float_array_unsigned.ser'
        from_file = _test_parse_primitive_array(filename)
        expected = [3.45, 34.55, 23.3]
        
        self.assertEqual(allclose(from_file, expected), True)

    def test_parse_prim_float_array_all_negative(self):
        filename = 'primitive_arrays/float_array_signed.ser'
        from_file = _test_parse_primitive_array(filename)
        expected = [-3.45, -34.55, -23.3]

        self.assertEqual(allclose(from_file, expected), True)

    def test_parse_prim_float_array_limits(self):
        filename = 'primitive_arrays/float_array_limits.ser'
        from_file = _test_parse_primitive_array(filename)
        expected = [0.0, 3.4**38, -3.4**38]

        self.assertEqual(allclose(from_file, expected), True)

    # 8 byte double arrays
    def test_double_array_all_negative(self):
        filename = "primitive_arrays/double_array_signed.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [-0.2343134, -1234132.3431, -312431.3, 0, -1.1234]

        self.assertEqual(allclose(from_file, expected), True)
    
     # 8 byte double arrays
    def test_double_array_all_positive(self):
        filename = "primitive_arrays/double_array_unsigned.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [0.2343134, 1234132.3431, 312431.3, 0, 1.1234]

        self.assertEqual(allclose(from_file, expected), True)

     # 8 byte double arrays
    def test_double_array_at_limits(self):
        filename = "primitive_arrays/double_array_limits.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [pow(2, -1074), 0, (2 - pow(2, -52))*pow(2, 1023), -float("inf"), float('inf')]
 
        self.assertEqual(allclose(from_file, expected), True)

     # 8 byte double arrays  (2-2^-52)·2^1023.
    def test_double_array_empty(self):
        filename = "primitive_arrays/double_array_empty.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = []

        self.assertEqual(allclose(from_file, expected), True)

     # 8 byte double arrays
    def test_double_array_new(self):
        filename = "primitive_arrays/double_array_new.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = []

        self.assertEqual(from_file, expected, True)


if __name__ == '__main__':
    unittest.main()