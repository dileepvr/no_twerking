import socket
import select
import sys
import random

class chat:
    def __init__(self):
        if len(sys.argv)>1:
            self.PNUM = int(raw_input("what port number do you want?"))
        else:
            self.PNUM = 12346

        self.S = socket.socket(socket.AF_UNIX,socket.SOCK_RAW)
        self.S.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.S.bind("" + ":" + str(self.PNUM))

        self.ADR = {}
        self.name = raw_input("username: ")
        temp = raw_input("What address do you want to send data to: ")

        if temp != "":
            temp = temp.split(":")
            if len(temp) == 2:
                if temp[1] == "scan":
                    self.scan_for_connection(socket.gethostbyname(temp[0]))
                else:
                    self.ADR[socket.gethostbyname(temp[0]) + ":" + str(int(temp[1]))] = ""
            else:
                self.ADR[socket.gethostbyname(temp[0]) + ":" + str(self.PNUM)] = ""

        self.S.settimeout(2)
        self.msg = ""

        self.internal_print("You may now converse\nType 'exit' and hit return to leave the chat\n")

    def scan_for_connection(self,IP):
        self.internal_print("Starting scan of IP " + str(IP) + "\n")
        for p in range(1,pow(2,16)):
            self.send("##scan##",(IP,p))
        self.internal_print("Finished scan of IP " + str(IP) + "\n")

    def send(self,mssg,addr,name = None):
        N = name
        if N == None:
            N = self.name
        temp = "#_#_p2gbot_#_#" + N + "#NAME#"

        temp = temp + mssg
        print(temp)
        self.S.sendto(temp,addr)

    def recieve(self,LL):
        temp = self.S.recvfrom(LL)

        if temp[0][:14]!= "#_#_p2gbot_#_#":
            return "","",False

        out = temp[0][14:].split("#NAME#")

        return out[1],out[0],temp[1]

    def internal_print(self,mssg,name = None,me = False):

        if me == True:
            sys.stdout.write("\033[F")
        
        sys.stdout.write("\033[K")
            
        #and write it with a name attached
        if name != None:
            sys.stdout.write("[" + name + "]")

        sys.stdout.write(mssg)
        sys.stdout.flush()        

    def chat(self):
        while self.msg != "exit\n":
            read,write,error = select.select([self.S,sys.stdin],[],[],0)
            for sock in read:
                if sock == self.S:
                    mssg,NAME,adr = self.recieve(1024)

                    #if adr == False then the message is not formatted properly, skip it
                    if adr != False:
                        #if its a new address, add it to the list of people in communication
                        if adr not in self.ADR.keys():
                            out = NAME + " at address " + str(adr) + " joined the chat\n"

                            self.internal_print(out)
                            
                            for a in self.ADR.keys():
                                self.send(out,a)
                            
                            for a in self.ADR.keys():
                                self.send("##newconn##" + str(adr[0]) + "," + str(adr[1]) + "," + NAME,a)
                                #send a message with the new name attached
                                self.send(mssg,a,NAME)

                            if len(self.ADR.keys()) > 0:
                                t = ""
                                for a in self.ADR.keys():
                                    t = t + "#" + str(a[0]) + "," + str(a[1]) + "," + self.ADR[a]
                                self.send("##newconn#" + t,adr)

                        #record the name
                        self.ADR[adr] = NAME

                        if mssg[:8] == "##scan##":
                            self.send("I got your scan\n",adr)

                        elif mssg[:11] == "##newconn##":
                            temp = mssg[11:].split("#")
                        
                            for t in temp:
                                a = t.split(",")

                                T2 = (socket.gethostbyname(a[0]),int(a[1]))
                                tname = a[2]
                                if T2 not in self.ADR.keys():
                                    self.ADR[T2] = tname

                        elif mssg == "exit\n":
                            self.internal_print(self.ADR[adr] + " has left the chat\n")
                            self.ADR.pop(adr)

                        else:
                            if NAME != self.name:
                                self.internal_print(mssg,NAME)
                else:
                    self.msg = sys.stdin.readline()

                    self.internal_print(self.msg,self.name,True)

                    for a in self.ADR.keys():
                        self.send(self.msg,a)
        self.S.close()

def main():
    ME = chat()
    ME.chat()

if __name__ == "__main__":
    main()
