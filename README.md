# Mini Project 1
#### In this readme, we will explain step by step how to connect data from sensors to the cloud and upload it there, via different commands that you will run. That data will be visualized in charts.

## Step 0: Access iot lab testbed

&nbsp;&nbsp;&nbsp;&nbsp;Link for how to generate SSH key and associate account to computer: https://www.iot-lab.info/docs/getting-started/ssh-access/

&nbsp;&nbsp;&nbsp;&nbsp;After following what it says in the link, we will do everything inside, for example, Grenoble's frontend:

```bash
$ ssh <login>@grenoble.iot-lab.info
```
## Step 1: Clone the Repository

```bash
$ git clone https://github.com/manexsora/mini-project-1
```
&nbsp;&nbsp;&nbsp;&nbsp;Access repository:
```bash
$ cd mini-project-1/
```

## Step 2: Clone RIOT Operating System
```bash
$ git clone https://github.com/RIOT-OS/RIOT.git -b 2020.10-branch
```

## Step 3: Set Up RIOT Environment
&nbsp;&nbsp;&nbsp;&nbsp;Activate and configure RIOT environment necessary for running software in it:
```bash
$ source /opt/riot.source
```
&nbsp;&nbsp;&nbsp;&nbsp;Compile the border router, so that the IoT network and external networks are connected:
```bash
$ make ETHOS_BAUDRATE=500000 DEFAULT_CHANNEL=5 BOARD=iotlab-m3 -C RIOT/examples/gnrc_border_router clean all
```
&nbsp;&nbsp;&nbsp;&nbsp;Access the node_code file, and with what we have there we will compile all the code we have:
```bash
$ cd node-code
$ make DEFAULT_CHANNEL=5 all
```

## Step 4: Create and Configure IoT-Lab Experiment
&nbsp;&nbsp;&nbsp;&nbsp;Submit an experiment to Iot-Lab testbed requesting five free nodes with m3 architecture and Grenoble as the site.
```bash
$ iotlab-experiment submit -n mini1 -d 60 -l 3,archi=m3:at86rf231+site=grenoble
```
&nbsp;&nbsp;&nbsp;&nbsp;View the activated nodes and their details, take into account the id of the nodes, will be important:
```bash
$ iotlab-experiment get -i 388084 -r
```
&nbsp;&nbsp;&nbsp;&nbsp;Flash router firmware, which means configuring one of the activated nodes as a border router (in our case the 95 is one of the id's seen in with the command before):
```bash
$ iotlab-node --flash RIOT/examples/gnrc_border_router/bin/iotlab-m3/gnrc_border_router.elf -l grenoble,m3,95
```
&nbsp;&nbsp;&nbsp;&nbsp;Assign an Ipv6 address to that node (if it says "Device or resource busy" choose a different tap; tap2, tap3, tap4... and another Ipv6 adress; between 2001:660:5307:3100::/64	and 2001:660:5307:317f::/64):
```bash
$ sudo ethos_uhcpd.py m3-95 tap1 2001:660:5307:3120::1/64
```
## Step 5: Flash sensor firmware
&nbsp;&nbsp;&nbsp;&nbsp;For this first create a new terminal, and in it connect to grenoble with the command of the step 0 and then access the mini-project-1:
```bash
$ ssh <login>@grenoble.iot-lab.info
$ cd mini-project-1/
```
&nbsp;&nbsp;&nbsp;&nbsp;Prepare the other nodes for their roles in the experiment, taking the compiled code of the second make command (take the id's of the nodes that weren't used for the border router):
```bash
$ iotlab-node --flash node_code/bin/iotlab-m3/mini-project.elf -l grenoble,m3,96+97
```
## Step 6: Connect to the Server:
&nbsp;&nbsp;&nbsp;&nbsp;Open a new terminal and in it connect to our server, which is an amazon aws server:
```bash
$ ssh -i "IOTLAB.pem" admin@ec2-51-20-254-148.eu-north-1.compute.amazonaws.com
```
&nbsp;&nbsp;&nbsp;&nbsp;Access the server_code, and instruct Docker to start running the coap server, which will get the data of the sensors and upload it on the cloud,  the mysql, which will create given tables with the data, and grafana, which will later visualize those tables:
```bash
$ cd server_code/
$ sudo docker compose up
```
## Step 6: Access Grafana:
&nbsp;&nbsp;&nbsp;&nbsp;Click on the link to get the data on Grafana: http://ec2-51-20-254-148.eu-north-1.compute.amazonaws.com:3000/public-dashboards/91a4ed78ec064712925d7cc58c83ecee?orgId=1&refresh=5s&from=now-5m&to=now
