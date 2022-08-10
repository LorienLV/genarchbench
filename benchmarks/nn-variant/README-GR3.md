# NN-Variant - Graviton3 - AWS

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
aws$ docker pull armswdev/tensorflow-arm-neoverse:r22.07-tf-2.9.1-onednn-acl_threadpool
aws$ sudo docker image ls
aws$ sudo docker run -it --init armswdev/tensorflow-arm-neoverse:r22.07-tf-2.9.1-onednn-acl_threadpool
docker$
```
Last, we need to install the NN-variant related libraries inside the docker instance:
```bash
docker$ sudo apt-get update
docker$ sudo apt-get install pip
docker$ sudo apt-get install libbz2-dev
docker$ sudo apt-get install liblzma-dev
docker$ sudo apt-get install libcurl4-gnutls-dev
docker$ sudo apt-get install time
docker$ sudo apt-get -y install pypy3
docker$ sudo apt-get install zlib1g
docker$ sudo apt-get install xz-utils  bzip2 automake curl
docker$ sudo apt-get install parallel
docker$ sudo apt-get install samtools
docker$ pip install blosc
docker$ pip install numpy
docker$ pip install tensorflow-addons
docker$ pip install mpmath
docker$ pip install intervaltree tables
docker$ pip install pigz-python
docker$ pip install zstd
docker$ pip install whatshap
docker$ pip install cffi

GNU-parallel from source:
```bash
docker$ wget --no-check-certificate https://ftp.gnu.org/gnu/parallel/parallel-latest.tar.bz2
docker$ tar -xf  parallel-latest.tar.bz2
docker$ rm parallel-latest.tar.bz2
docker$ cd parallel-20220722/
docker$ ./configure
docker$ make
docker$ sudo make install
```

In this stage, the docker instance is ready to run the NN-variant application with all it's dependencies.
It is highly recomended to commit the changes we have done on the image. otherwise, upon closing the docker session, all our changes will be lost. 

```bash
aws$ sudo docker ps
# This will show you the current running docker sessions with container_ID
aws$ sudo docker commit <container_ID> <new_name>
aws$ sudo docker run -v /<data>:/<data> -it --init <new_name>
```
For more information, see below [AWS and Docker cheat sheet](#AWS-and-Docker-cheat-sheet)

>For a newer Tenserflow version check here: <https://hub.docker.com/r/armswdev/tensorflow-arm-neoverse>

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

