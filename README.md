# Mini Project 1
#### In this readme, we will explain step by step how to connect data from sensors to the cloud and upload it there, via different commands that you will run. That data will be visualized in charts.

## Step 1: Clone the Repository

```bash
git clone https://github.com/manexsora/mini-project-1
cd mini-project-1
```

## Step 2: Clone RIOT Operating System
```bash
git clone https://github.com/RIOT-OS/RIOT
```
## Step 3: Set Up RIOT Environment
```bash
# Activate RIOT environment (before compiling)
source /opt/riot.source

# Compile border router
make ETHOS_BAUDRATE=500000 DEFAULT_CHANNEL=5 BOARD=iotlab-m3 -C RIOT/examples/gnrc_border_router clean all
cd node-code
# Compile the code
make DEFAULT_CHANNEL=5 all
```
## Step 4: Create and Configure IoT-Lab Experiment
```bash
# Create experiment
iotlab-experiment submit -n mini1 -d 60 -l 5,archi=m3:at86rf231+site=grenoble

# View nodes
iotlab-experiment get -i 388084 -r

# Flash router firmware
iotlab-node --flash RIOT/examples/gnrc_border_router/bin/iotlab-m3/gnrc_border_router.elf -l grenoble,m3,95
#95 being one of our 5 nodes that we have activated

# Change tap IP address
sudo ethos_uhcpd.py m3-95 tap1 2001:660:5307:3120::1/64
```
## Step 5: Flash sensor firmware
For this first create a new terminal
```bash
cd mini-project-1/
iotlab-node --flash node_code/bin/iotlab-m3/mini-project.elf -l grenoble,m3,96+97+101+102
```
## Step 6: Connect to the Server:
Open a new terminal to connect to our server
```bash
ssh -i "IOTLAB.pem" admin@ec2-51-20-254-148.eu-north-1.compute.amazonaws.com
cd server_code/
# Start the server
sudo docker-compose up
```
## Step 6: Access Grafana:

