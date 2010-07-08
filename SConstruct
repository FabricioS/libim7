import os
# Building zlib
zlib_dir = './src/zlib/'
zlib_src = [os.path.join(zlib_dir,tmp) for tmp in os.listdir(zlib_dir) if tmp.endswith('.c')]

obj_z = []
for src in zlib_src:
    obj_z.append(Object(src))

# Building ReadIM7
im7_dir = './src/'
env = Environment(CCFLAGS='-D_WIN32 -DBUILD_DLL', CPPPATH=zlib_dir)
obj_7 = env.Object(os.path.join(im7_dir,'ReadIM7.cpp'))
obj_X = env.Object(os.path.join(im7_dir,'ReadIMX.cpp'))
SharedLibrary(target='_im7.dll', source=obj_z+[obj_7, obj_X])