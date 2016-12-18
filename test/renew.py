#!/usr/bin/python

#Expected behavior: all 20 packets should be allowed through

import time
import sys
import shutil
from core import netns
from core import pycore

def ncmd(node, args):
  return node.redircmd(sys.stdin.fileno(),sys.stdout.fileno(),sys.stdout.fileno(),args)

session = pycore.Session(persistent=True)

ptpnet1 = netns.nodes.PtpNet(session)
ptpnet2 = netns.nodes.PtpNet(session)

n1 = session.addobj(cls = pycore.nodes.CoreNode, name="n1")
router = session.addobj(cls = pycore.nodes.CoreNode, name="router")
n2 = session.addobj(cls = pycore.nodes.CoreNode, name="n2")
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

ncmd(router,["sudo","./ngp","start"])
ncmd(n1,["sudo","./ngp","start"])

ncmd(n1,["ping", "-c", "20", "10.0.2.10"])

ncmd(n1,["sudo","./ngp","stop"])
ncmd(router,["sudo","./ngp","stop"])

ncmd(router,["sudo","cat","/var/log/network_gatekeeper.log"])
ncmd(n1,["sudo","cat","/var/log/network_gatekeeper.log"])

session.shutdown()
