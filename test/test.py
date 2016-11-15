#!/usr/bin/python

import time
from core import netns
from core import pycore

session = pycore.Session(persistent=True)

ptpnet1 = netns.nodes.PtpNet(session)
ptpnet2 = netns.nodes.PtpNet(session)

n1 = session.addobj(cls = pycore.nodes.CoreNode, name="n1")
n1.setposition(x=850.0,y=381.0)
router = session.addobj(cls = pycore.nodes.CoreNode, name="router")
router.setposition(x=528.0,y=100.0)
n2 = session.addobj(cls = pycore.nodes.CoreNode, name="n2")
n2.setposition(x=275.0,y=372.0)
router.newnetif(net=ptpnet1, addrlist=["10.0.1.1/24"], ifindex=0)
router.newnetif(net=ptpnet2, addrlist=["10.0.2.1/24"], ifindex=1)
n1.newnetif(net=ptpnet1, addrlist=["10.0.1.10/24"], ifindex=0)
n2.newnetif(net=ptpnet2, addrlist=["10.0.2.10/24"], ifindex=0)

n1.icmd(["ip","route","add","default","via","10.0.1.1"])
n2.icmd(["ip","route","add","default","via","10.0.2.1"])

router.nodefilecopy("ngp","../ngp")
router.nodefilecopy("net.conf","../net.conf")
n1.nodefilecopy("ngp","../ngp")
n1.nodefilecopy("net.conf","../net.conf")

router.icmd(["./ngp","start"])
n1.icmd(["./ngp","start"])
time.sleep(1)
n1.icmd(["ping", "-c", "5", "10.0.2.10"])

time.sleep(1)

n1.icmd(["./ngp","stop"])
router.icmd(["./ngp","stop"])

time.sleep(2)

#router.icmd(["cat","/var/log/network_gatekeeper.log"])
#n1.icmd(["cat","/var/log/network_gatekeeper.log"])

time.sleep(3)

session.shutdown()
