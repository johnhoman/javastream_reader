from distutils.core import setup, Extension

extension_mod = Extension("jso_reader", ["jso_reader.c"], undef_macros=['NDEBUG'])
setup(name="jso_reader", ext_modules=[extension_mod])
