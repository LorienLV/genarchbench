# NN-BASE - AWS Graviton3 Ubuntu 22.04

## Environment Setup
After you have logged in to the AWS node, you need to install the required packages.
For clarity, `aws$` stands for bash in the aws node, where `docker$` stands for the bash inside the running docker instance.

First, install the supporting packages to run the docker environment on the machine:
```bash
aws$ sudo apt-get install ca-certificates curl gnupg lsb-release
aws$ sudo mkdir -p /etc/apt/keyrings
aws$ curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
aws$ echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
```
Second, install the docker environment and download the latest pytorch avaiable with the openblas backend:
```bash
aws$ sudo apt-get update
aws$ sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin
aws$ sudo docker run hello-world
aws$ # sudo docker pull armswdev/pytorch-arm-neoverse:r22.07-torch-1.12.0-openblas
aws$ sudo docker pull armswdev/pytorch-arm-neoverse:r22.07-torch-1.12.0-onednn-acl
aws$ sudo docker image ls
aws$ # sudo docker run -it --init armswdev/pytorch-arm-neoverse:r22.07-torch-1.12.0-openblas
aws$ sudo docker run -it --init armswdev/pytorch-arm-neoverse:r22.07-torch-1.12.0-onednn-acl
docker$
```
Last, we need to install the NN-Base related libraries inside the docker instance:
```bash
docker$ pip install toml
docker$ pip install scipy
docker$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
docker$ source "$HOME/.cargo/env"
docker$ git clone https://github.com/nanoporetech/ont_fast5_api
docker$ pip install ./ont_fast5_api
docker$ git clone https://github.com/nanoporetech/fast-ctc-decode.git
docker$ pip install ./fast-ctc-decode/
```
In this stage, the docker instance is ready to run the NN-Base application with all it's dependencies.
It is highly recomended to commit the changes we have done on the image. otherwise, upon closing the docker session, all our changes will be lost. 

```bash
aws$ sudo docker ps
# This will show you the current running docker sessions with container_ID
aws$ sudo docker commit <container_ID> <new_name>
aws$ sudo docker run -v /<data>:/<data> -it --init --security-opt --security-opt seccomp=/data/genarch-bench/docker_security.json <new_name>
docker images
```
For more information, see below [AWS and Docker cheat sheet](#AWS-and-Docker-cheat-sheet)

>For a newer pytorch version check here: <https://hub.docker.com/r/armswdev/pytorch-arm-neoverse>

#### AWS and Docker cheat sheet
Connect to AWS machine:
```sh
local$ ssh -i $AWSKEY aws_sessionID
```
Copy from local machine to AWS machine:
```sh
local$ scp -i $AWSKEY ./path/to/files/ aws_sessionID:/path/to/destination
```
Run docker image:
```sh
aws$ sudo docker run -it --init <docker_name>
```
See all available dockers:
```sh
aws$ sudo docker ls
```
See all running dockers on the node:
```sh
aws$ sudo docker ps
```
Save the docker image state:
```sh
# Open another another ssh session to the aws node and run:
aws$ sudo docker ps
# This will show you the current running docker sessions with container ID
# Now commit chamges (possible to add comment with -m <comment>)
aws$ sudo docker commit <container_ID> <new_name>
```
> Note: Next time you will need to run the container with the `$<new_name>`

Copy from AWS machine to Docker image:
```sh
aws$ sudo docker cp -rp <from> <docker_id>:path
```

