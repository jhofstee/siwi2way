WHAT IT IS
==========

This project tries to use http keep-alive connection to create
a virtual AT terminal which will also function when the device
is behind a NAT / firewall.

Although the project does work on windows it is created for grps
modems from Sierra Wireless, http://www.sierrawireless.com/. Since
gprs networks are typically NAT networks, it is not (easily) 
possible to setup a tcp connection to the modem.

By using PubNub http://www.pubnub.com this is circumvented.
an alternative would be http://www.pusher.com but is not implemented.


TO USE
======

in bin are prebuild binaries. 

Siwi2way.exe for Windows.

siwi2way_oat_2_36_0_ethernet.dwl 
for the FXT with the ethernet IESM card. It does not support gprs!

create a account at http://www.pubnub.com

# be verbose
at+vind=1

# but not about regs
at+vind=0,reg

# store it
at+vind=2

# show settings
at+vreg=1

# set the keys (actual values from the pubnub account)
at+vreg=2,1,"pub-xxxxxx-xxxx"
at+vreg=2,2,"sub-xxxxxx-xxxx"

#check
at+vreg=1

#reset (or restart exe)
at+cfun

login to pubnub, goto to 'Dev console' and press subscribe

Type e.g. (mind the quotes) and press send message
"at+vind"


TO BUILD
========
mkdir some/source/dir
cd some/source/dir
git clone .... 
cd yajl
git clone https://github.com/lloyd/yajl.git ext
# I am at 8b48967c74e9b16c07f120b71598f5e5269e8f57 at the time of writing

#in a visual studio shell go to that dir and type
cmake -G "Visual Studio 10" ext

- create an empty workspace in Open AT developer studio
- disable 'build automatically'
- create dummy wip project (e.g. tcp client example)
- move the bearer init from the dummy to the ethernet_driver.lib (and set config if needed)


- import -> existing project into workspace
- make sure copy projects into project workspace is not checked
- select C:\software\siwi2way\siwi2way, project should be marked after selecting
- press finish
- check wip / os dependencies
- siwi2way -> properties -> C/C++ general -> Paths and symbols
  set these to the correct location
	C:\software\vgr\firmware\inc
	C:\software\vgr\yajl\yajl-2.0.5\include
- select target arm_release
- press build ...
- right top -> target management
- keep devices and TMConsole, close all others
- in devices select the com port, hit connect (the blue icon)
- press download a dwl file, select the build dwl.

- add a wip project to get the bearer startup code

Jeroen Hofstee (jhofstee@victronenergy.com)
Victron Energy B.V. (http://www.victronenergy.com)

A word of thanks:
Doug Lea for his malloc. http://g.oswego.edu/dl/html/malloc.html
Lloyd Hilaiel and contributors for yajl, http://lloyd.github.com/yajl/
