FROM i386/ubuntu:14.04
LABEL   maintainer="Maojui <maojui0427@gmail.com>" \
        version="0.0.1" \
        date="2018-05-08" 
# ============ Start Installation  ============
RUN apt update && apt install -y \
    ed \
    g++ \
    csh \
    make \
    wget \
    && rm -rf /var/lib/apt/lists/*
# Get the nashOS source
RUN wget https://github.com/maojui/nachOS/blob/master/nachos-4.0.tar.gz?raw=true -O nachos-4.0.tar.gz
RUN wget https://github.com/maojui/nachOS/blob/master/nachos-lib.tar?raw=true -O nachos-lib.tar
# Unzip
RUN tar -zxvf nachos-4.0.tar.gz && tar -xvf nachos-lib.tar
# Put them into right place
RUN mv nachos /usr/local/nachos
# Compile
RUN cd nachos-4.0/code && make
# ============ End Installation  ============