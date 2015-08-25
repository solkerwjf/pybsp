import bsp
import numpy
import math

nProcs = bsp.procCount()
myProcID = bsp.myProcID()
partnerID = 1^myProcID
mSync = numpy.ndarray([nProcs,nProcs],dtype=bool)
for row in range(nProcs):
    for col in range(nProcs):
        if col == 1^row:
            mSync[row][col] = True
        else:
            mSync[row][col] = False

s1 = [1,(2,3,4),[5,(5,6)],{7:[1,2],8:9}]
s2 = (3,5,'123','hello world!')
bsp.fromObject(s1,'obj.s1')
bsp.fromObject(s2,'obj.s2')

a1 = numpy.ndarray([2,3,3],dtype='f8')
a2 = numpy.ndarray([10])
for i in range(3):
    for j in range(3):
        a1[0][i][j] = i+j
        a1[1][i][j] = i-j
for k in range(10):
    a2[k] = math.cos(k)
bsp.fromNumpy(a1,'num.a1')
bsp.fromNumpy(a2,'num.a2')

bsp.toProc(partnerID,'obj.s1')
bsp.toProc(partnerID,'obj.s2')
bsp.toProc(partnerID,'num.a1')
bsp.toProc(partnerID,'num.a2')

bsp.sync('hello',mSync)
t1=bsp.toObject(bsp.fromProc(partnerID)['obj.s1'])
print 't1 = ', t1
t2=bsp.toObject(bsp.fromProc(partnerID)['obj.s2'])
print 't2 = ', t2
b1=bsp.toNumpy(bsp.fromProc(partnerID)['num.a1'])
b2=bsp.toNumpy(bsp.fromProc(partnerID)['num.a2'])
print 'b1 = ', b1
print 'b2 = ', b2

