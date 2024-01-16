# Mini Project 2
#### In this readme, we will explain step by step how to connect data from sensors to the cloud, doing a preprocess of that data where we try to smooth it out and recieve the correct data to the cloud. Also, the data won't be just recieved by one site as before, in this case we will get data from three different sites and three different boars in each site. 

## Step 0: Access iot lab testbed

&nbsp;&nbsp;&nbsp;&nbsp;Link for how to generate SSH key and associate account to computer: https://www.iot-lab.info/docs/getting-started/ssh-access/

&nbsp;&nbsp;&nbsp;&nbsp;After following what it says in the link, we will do everything inside, for example, in Grenoble's frontend. We could do this next steps in each one of the three sites also, but it's not necessary and it would take a lot of time:

```bash
$ ssh <login>@grenoble.iot-lab.info
```
## Step 1: Clone the Repository

```bash
$ git clone https://github.com/josebagabilondo/mini-project-2
```
&nbsp;&nbsp;&nbsp;&nbsp;Access repository:
```bash
$ cd mini-project-2/
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
$ cd node_code/
$ make DEFAULT_CHANNEL=5 all
```

## Step 4: Create and Configure IoT-Lab Experiment
&nbsp;&nbsp;&nbsp;&nbsp;Submit an experiment to Iot-Lab testbed requesting five free nodes with m3 architecture and Grenoble as the site.
```bash
$ iotlab-experiment submit -n mini1 -d 60 -l 3,archi=m3:at86rf231+site=grenoble
```
&nbsp;&nbsp;&nbsp;&nbsp;View the activated nodes and their details, take into account the id of the nodes, will be important. Take into account that the number of your experiment (mine being 388084) will be the id you get after running the command before:
```bash
$ iotlab-experiment get -i 388084 -r
```
&nbsp;&nbsp;&nbsp;&nbsp;Flash router firmware, which means configuring one of the activated nodes as a border router (in our case the 95 is one of the id's seen in with the command before). Before doing it go back to mini-project-1 file:
```bash
$ cd ..
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
&nbsp;&nbsp;&nbsp;&nbsp;Open a new terminal and in it connect to our server, which is an amazon aws server. For that, first download a file if don't already have it called iotlab.pem (you can find it inside the github and download it), where there is a public key to be able to connect to the server. We will have to access the place where the file is also before connecting to the server:
```bash
$ cd Downloads/
$ ssh -i "IOTLAB.pem" admin@ec2-51-20-254-148.eu-north-1.compute.amazonaws.com
```
&nbsp;&nbsp;&nbsp;&nbsp;Access the server_code, and instruct Docker to start running the coap server, which will get the data of the sensors and upload it on the cloud,  the mysql, which will create given tables with the data, and grafana, which will later visualize those tables:
```bash
$ cd mini-project-1/server_code/
$ sudo docker compose up
```
## Step 6: Access Grafana:
&nbsp;&nbsp;&nbsp;&nbsp;Click on the link to get the data on Grafana: [http://ec2-51-20-254-148.eu-north-1.compute.amazonaws.com:3000/public-dashboards/91a4ed78ec064712925d7cc58c83ecee?orgId=1&refresh=5s&from=now-5m&to=now ](http://ec2-51-20-254-148.eu-north-1.compute.amazonaws.com:3000/d/f8b2ba85-fd5c-482d-94e1-539f93f85b1b/berria?orgId=1&from=1703806595000&to=1703809330000) 

&nbsp;&nbsp;&nbsp;&nbsp;In it there is data given by different sensors at real time, for temperature pressure and light, and also the messages recived until now and how many node we've activated in the process of creating the mini project 1.

## Video:
&nbsp;&nbsp;&nbsp;&nbsp;[https://youtu.be/HeRUO_gKO2A](https://youtu.be/8kjjtQoug-o)https://youtu.be/8kjjtQoug-o
