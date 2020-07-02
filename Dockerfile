FROM buildpack-deps:jessie

# Download sources
RUN cd /opt && curl -LO http://fallabs.com/kyotocabinet/pkg/kyotocabinet-1.2.76.tar.gz
RUN cd /opt && tar -xvzf kyotocabinet-1.2.76.tar.gz && mv kyotocabinet-1.2.76 kyotocabinet && rm kyotocabinet-1.2.76.tar.gz
RUN apt-get update

# Install clang
RUN apt-get install -y clang
ENV C clang
ENV CXX clang++

# Install kyoto cabinet
RUN apt-get -y install liblzo2-dev liblzma-dev zlib1g-dev build-essential
RUN cd /opt/kyotocabinet && ./configure –enable-zlib –enable-lzo –enable-lzma && make && make install

# install raptor2
RUN apt-get install -y libraptor2-dev

# Install Serd
RUN apt-get install -y libserd-dev

# Install CMake
RUN curl -sSL https://cmake.org/files/v3.5/cmake-3.5.2-Linux-x86_64.tar.gz | tar -xzC /opt
ENV PATH /opt/cmake-3.5.2-Linux-x86_64/bin/:$PATH

# Install GDB for debugging
RUN apt-get install -y gdb

# Copy sources
COPY deps /opt/cobra/deps
COPY ext /opt/cobra/ext
COPY src /opt/cobra/src
COPY CMakeLists.txt /opt/cobra/CMakeLists.txt
COPY run.sh /opt/cobra/run.sh
COPY run-debug.sh /opt/cobra/run-debug.sh

# Enable optional dependencies in Makefile
RUN cd /opt/cobra/deps/hdt/hdt-lib && sed -i "s/#KYOTO_SUPPORT=true/KYOTO_SUPPORT=true/" Makefile

RUN mkdir /opt/cobra/build
RUN cd /opt/cobra/build && cmake .. -Wno-deprecated
RUN cd /opt/cobra/build && make

WORKDIR /var/evalrun

# Default command
ENTRYPOINT ["/opt/cobra/run.sh"]
CMD ["/var/patches", "1", "58", "/var/queries"]