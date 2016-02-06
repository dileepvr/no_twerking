import socket
import select
import random

S = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
S.bind(("",12345))

PORT = random.randint(1025,pow(2,11))
print(PORT)
Sr = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
Sr.bind(("",PORT))

while 1:
    read,write,error = select.select([S,Sr],[],[],0)
    
    for s in read:
        if s == Sr:
            print("From random port: " + str(PORT))
        print(s.recvfrom(1024))
    
