# NN-VARIANT - AWS Graviton3 Ubuntu

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
aws$ sudo apt-get update
aws$ sudo apt-get install docker-ce docker-ce-cli containerd.io docker-compose-plugin
```

Second, download the latest pytorch avaiable, here we have three options as discribe in the [ARM docker website](https://hub.docker.com/r/armswdev/tensorflow-arm-neoverse) (please check of updates):

| Tag |	TensorFlow version |	oneDNN version |	Compute Library version	| Description |
|:---:|:---:|:---:|:---:|--- |
|latest|	2.9.1|	2.6| 22.05|	Points to the **recommended** onednn-acl_threadpool image (below).|
|r22.08-tf-2.9.1-onednn-acl|	2.9.1|	2.6|	22.05|	TensorFlow is compiled with oneDNN optimized with ACL, using ACL's own scheduler.|
|r22.08-tf-2.9.1-onednn-acl_threadpool | 2.9.1 |2.6	| 22.05 | **(Recommended)** TensorFlow is compiled with oneDNN optimized with ACL, using TensorFlow's Eigen threadpool for parallelism.|
|r22.08-tf-2.9.1-eigen|	2.9.1|	N/A|	N/A	|TensorFlow is compiled with the Eigen backend.|

Now, we can download the docker image that we want using the <image tag> from the table above: ``` aws$ sudo docker pull armswdev/tensorflow-arm-neoverse:<image tag>```

We found out that NN-variant perform the best while using TensorFlow with the Eigen backend.
Therefore, we continue our explanation using the Eigen version (<image tag> = r22.08-tf-2.9.1-eigen):
```bash
aws$ docker pull armswdev/tensorflow-arm-neoverse:r22.08-tf-2.9.1-eigen
aws$ sudo docker run -it --init armswdev/tensorflow-arm-neoverse:r22.08-tf-2.9.1-eigen
docker$
```

Alternativly we can use:
```bash
aws$ docker pull armswdev/tensorflow-arm-neoverse:r22.07-tf-2.9.1-onednn-acl_threadpool
aws$ sudo docker run -it --init armswdev/tensorflow-arm-neoverse:r22.07-tf-2.9.1-onednn-acl_threadpool
# Optional flags oneDNN+ACL
# export TF_ENABLE_ONEDNN_OPTS=0
# export TF_ENABLE_ONEDNN_OPTS=1 # enables the oneDNN backend
#export ARM_COMPUTE_SPIN_WAIT_CPP_SCHEDULER=0
#export ARM_COMPUTE_SPIN_WAIT_CPP_SCHEDULER=1
```
OR 
```bash
aws$ docker pull armswdev/tensorflow-arm-neoverse:r22.08-tf-2.9.1-onednn-acl
sudo docker run -it --init armswdev/tensorflow-arm-neoverse:r22.08-tf-2.9.1-onednn-acl
# Optional flags oneDNN+ACL
# export TF_ENABLE_ONEDNN_OPTS=0
# export TF_ENABLE_ONEDNN_OPTS=1 # enables the oneDNN backend
```

Last, we install the NN-variant related libraries inside the docker instance:
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
docker$ pip install blosc
docker$ pip install numpy
docker$ pip install tensorflow-addons
docker$ pip install mpmath
docker$ pip install intervaltree tables
docker$ pip install pigz-python
docker$ pip install zstd
docker$ pip install whatshap
docker$ pip install cffi
```

samtools and GNU-Parallel are two libraries that require manual installation from source as they are not compiled by default to ARM instructions set.
Start with samtool:
```bash
docker$ mkdir -p /home/ubuntu/tools && cd /home/ubuntu/tools
docker$ wget https://github.com/samtools/samtools/archive/refs/tags/1.10.zip
docker$ wget https://github.com/samtools/htslib/archive/refs/tags/1.10.zip
docker$ unzip 1.10.zip
docker$ unzip 1.10.zip.1
docker$ rm 1.10.zip 1.10.zip.1
docker$ cd samtools-1.10
docker$ autoheader            # Build config.h.in (this may generate a warning about
docker$                       # AC_CONFIG_SUBDIRS - please ignore it).
docker$ autoconf -Wno-syntax  # Generate the configure script
docker$ ./configure --prefix=/home/ubuntu/tools/samtools-1.10/install/ --with-htslib=/home/ubuntu/tools/htslib-1.10/ --without-curses     # Needed for choosing optional functionality
docker$ make
docker$ make install
docker$ echo "export PATH=/home/ubuntu/tools/samtools-1.10/install/bin:$PATH" >> ~/.bashrc
docker$ source ~/.bashrc 
```
Continue with GNU-parallel:
```bash
docker$ wget --no-check-certificate https://ftp.gnu.org/gnu/parallel/parallel-latest.tar.bz2
docker$ tar -xf  parallel-latest.tar.bz2
docker$ rm parallel-latest.tar.bz2
docker$ cd parallel-20220822/
docker$ ./configure --prefix=/home/ubuntu/tools/parallel-20220822/install/
docker$ make 
docker$ make install
# add to $PATH ~/.bashrc, Make sure you add the new path to the beginning of the path environment variable
docker$ echo "export PATH=/home/ubuntu/tools/parallel-20220822/install/bin:$PATH" >> ~/.bashrc
docker$ source ~/.bashrc 
```

In this stage, the docker instance is ready to run the NN-variant application with all it's dependencies.
It is highly recomended to commit the changes we have done on the image. otherwise, upon closing the docker session, all our changes will be lost. 

```bash
aws$ sudo docker ps
# This will show you the current running docker sessions with container_ID
aws$ sudo docker commit <container_ID> <new_name>
aws$ sudo docker run -v /<data>:/<data> -it --init <new_name>
docker$ source ~/.bashrc
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


