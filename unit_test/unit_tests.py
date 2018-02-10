"""

"""

import unittest
from numpy import allclose
from pprint import pprint
from os.path import join
from math import pow
from jso_reader import (
    _test_parse_primitive_array, 
    _test_parse_class_descriptor,
    stream_read
)

from os import system, pardir
import sys

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
        expected = [0.0]*300

        self.assertEqual(from_file, expected, True)

    def test_double_array_unsigned_2d(self):
        filename = "primitive_arrays/double_array_unsigned_2d.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [
            [0.2343134, 1234132.3431, 312431.3, 0, 1.1234],
            [1.2343134, 12342.003431, 31431.3, 0, 10.1234]
        ]
        self.assertEqual(from_file, expected)
    
    def test_double_array_unsigned_3d(self):
        filename = "primitive_arrays/double_array_unsigned_3d.ser"
        from_file = _test_parse_primitive_array(filename)
        expected = [
           [[0.2343134, 1234132.3431, 312431.3, 0, 1.1234],
            [1.2343134, 12342.003431, 31431.3, 0, 10.1234]],
           [[0.2343134, 1234132.3431, 312431.3, 0, 1.1234],
            [1.2343134, 12342.003431, 31431.3, 0, 10.1234]]
        ]

        self.assertEqual(from_file, expected)


class TestParsePrimitiveWrappers(unittest.TestCase):

    def test_class_descriptor_java_lang_Double(self):
        filename = "primitive_wrappers/double_wrapper.ser"
        from_file = _test_parse_class_descriptor(filename)
        expected = 10.0

        self.assertEqual(from_file, expected)
    
    def test_class_descriptor_java_lang_Double_array(self):
        filename = "primitive_wrappers/double_wrapper_array.ser"
        from_file = _test_parse_class_descriptor(filename)
        expected = [
           0,
           1,
           2,
           3,
           4,
           5,
           6,
           7,
           8,
           9,
        ]
        self.assertEqual(from_file, expected)

    def test_class_descriptor_java_lang_String(self):
        filename = "primitive_wrappers/string_single_sentence.ser"
        from_file = _test_parse_class_descriptor(filename)
        
        self.assertEqual(from_file, "This is a string that I am saving to a .ser file")


class TestComplexClassStructures(unittest.TestCase):

    def test_class_descriptor_simple_class(self):
        filename = "object_w_nested_object.ser"
        from_file = stream_read(filename)
        expected = {
            'firstName': 'jack',
            'lastName': 'homan',
            'ssn': 123456,
            'age': 27,
            'siblings': [{
                'firstName': 'ben',
                'lastName': 'homan',
                'ssn': 2345678,
                'age': 28,
                'siblings': []

            }]
        }
        self.assertDictEqual(from_file, expected)

    def test_class_descriptor_simple_class_1(self):
        filename = "object_w_nested_object_1.ser"
        from_file = stream_read(filename)
        expected = {
            'firstName': 'jack',
            'lastName': 'homan',
            'ssn': 123456,
            'age': 27,
            'array': [0]*200,
            'siblings': [{
                'firstName': 'ben',
                'lastName': 'homan',
                'ssn': 2345678,
                'age': 28,
                'siblings': [],
                'array': [0]*20
            }]
        }
        self.maxDiff = None
        self.assertDictEqual(from_file, expected)

    def test_class_descriptor_simple_class_2(self):
        filename = "object_w_nested_object_2.ser"
        from_file = stream_read(filename)
        expected = {
            'firstName': 'jack',
            'lastName': 'homan',
            'ssn': 123456,
            'age': 27,
            'array': [0]*10,
            'siblings': [{
                'firstName': 'ben',
                'lastName': 'homan',
                'ssn': 2345678,
                'age': 28,
                'siblings': [],
                'array': [0]*5
            }, 
            {
                'firstName': 'kaila',
                'lastName': 'mcnamara',
                'ssn': 98765,
                'age': 13,
                'siblings': [],
                'array': [0]*10
            },
            {
                'firstName': 'ashley',
                'lastName': 'mcnamara',
                'ssn': 9823702,
                'age': 22,
                'siblings': [],
                'array': [0]*10
            },
            
            ]
        }
        self.maxDiff = None
        self.assertDictEqual(from_file, expected)

    def test_super_class_parser(self):

        filename = "object_w_parent_class.ser"
        from_file = stream_read(filename)
        expected = {
            'firstName': 'jim',
            'lastName': 'beam',
            'ssn': 83838,
            'age': 33,
            'array': [0]*10,
            'siblings': [],
            'salary': 94399.00,
            'address': "2222 Aberdeen Dr. Mount Laurel, NJ 08054"
        }

        self.assertDictEqual(from_file, expected)

    def test_2_level_hierarchy(self):
        filename = "object_w_parent_class_with_parent_class.ser"
        from_file = stream_read(filename)
        expected = {
            'firstName': 'michael',
            'lastName': 'scott',
            'ssn': 9393920,
            'age': 29,
            'array': [0]*10,
            'siblings': [],
            'coworkers': [],
            'salary': 101022.00,
            'address': "2340 Aberdeen Dr. Mount Laurel, NJ 08054",
            'yearEndBonus': 0.0,
            'project': 'AWS'
        }

        self.assertDictEqual(from_file, expected)

    def test_aggragate_class_in_arraylist(self):
        filename = "object_w_parent_class_and_arraylist_aggregate.ser"
        from_file = stream_read(filename)
        print()
        print()
        pprint(from_file)

    def test_overloaded_privates(self):
        filename = "object_w_parent_class_with_parent_class_w_privates.ser"
        from_file = stream_read(filename)
        print()
        print()
        pprint(from_file)

class ContainerUnitTest(unittest.TestCase):

    _path = "containers"

    def test_bit_set(self):

        filename = join(self._path, "bit_set.ser")
        from_file = stream_read(filename)
        
        self.assertEqual(from_file, {0, 2, 4, 6, 8})

    def test_bit_set_1(self):
        filename = join(self._path, "bit_set_1.ser")
        from_file = stream_read(filename)
        expected = {i for i in range(100) if (i % 2) == 0}
        
        self.assertEqual(from_file, expected)

    def test_bit_set_2(self):
        filename = join(self._path, "bit_set_2.ser")
        from_file = stream_read(filename)
        expected = {
            i * 3 for i in range(100) 
            if (i * 3) % 2 == 0 or i % 17 == 0 or i % 13 == 0
        }
 
        self.assertEqual(from_file, expected)


    def test_linked_list_integer(self):
        filename = join(self._path, "linked_list_integer.ser")
        from_file = stream_read(filename)
        expected = [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

        self.assertEqual(from_file, expected)

    def test_hash_map_Integer_String(self):
        filename = join(self._path, "hash_map_integer.ser")
        from_file = stream_read(filename)
        expected = {
            0: '0',
            1: '1',
            4: '16',
            9: '81',
            16: '256',
            25: '625',
            36: '1296',
            49: '2401',
            64: '4096',
            81: '6561'
        }

        self.assertDictEqual(from_file, expected)

    def test_array_list_Integer(self):
        filename = join(self._path, "array_list_integer.ser")
        from_file = stream_read(filename)
        expected = [i**3 for i in range(10)]

        self.assertEqual(from_file, expected)

    def test_priority_queue_integer(self):
        filename = join(self._path, "priority_queue_integer.ser")
        from_file = stream_read(filename)
        expected = [i**4 for i in range(10)]

        self.assertEqual(from_file, expected)

    def test_hash_set_empty(self):
        filename = join(self._path, "hash_set_string.ser")
        from_file = stream_read(filename)
        expected = set()

        self.assertEqual(from_file, expected)

    def test_class_with_multiple_collections(self):
        filename = join(self._path, "full_class_containers.ser")
        from_file = stream_read(filename)

        expected = {
            'arrayList': [0, 1, 8, 27, 64],
            'bitSet': {0, 12, 6},
            'linkedList': [0, 1, 4, 9, 16],
            'hashMap': {
                0: '0', 
                1: '1', 
                4: '16', 
                9: '81', 
                16: '256'
            },
            'hashSet': set(),
            'queue': [0, 1, 16, 81, 256]
        }  
        self.assertDictEqual(from_file, expected)


if __name__ == '__main__':
    unittest.main()