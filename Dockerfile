FROM ubuntu:22.04
WORKDIR /parallel_computing/

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y curl build-essential libopenmpi-dev openmpi-bin mpich
RUN apt-get install -y openmpi-common libssl-dev 
COPY . .

CMD [ "/bin/bash" ]
# docker build -t parallel_computing .
# docker run -it --name=parallel_computing -v $(pwd):/parallel_computing parallel_computing