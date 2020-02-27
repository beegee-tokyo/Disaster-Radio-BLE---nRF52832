#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef SIM
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <openssl/sha.h>

struct timeval to_sleep;
serial Serial;

// from https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }
    /* Compute the time remaining to wait.
        tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

Layer1Class::Layer1Class() :

    _localAddress(),
    _transmitting(0),
    _nodeID(),
    _hashTable(),
    _hashEntry(0),
    _timeDistortion(1)
    //_loraInitialized(0),

{

}

int Layer1Class::nsleep(unsigned int secs, useconds_t usecs) {

  to_sleep.tv_sec = secs;
  to_sleep.tv_usec = usecs;
  
  return 0;
}

// 1 second in the simulation == 1 second in real life * timeDistortion
int Layer1Class::setTimeDistortion(float newDistortion){
    _timeDistortion = newDistortion;
	return 0;
}

float Layer1Class::timeDistortion(){
    return _timeDistortion;
}

int Layer1Class::simulationTime(int realTime){
    return realTime * timeDistortion();
}

int Layer1Class::getTime(){
    return time(NULL);
}

int Layer1Class::isHashNew(uint8_t hash[SHA1_LENGTH]){
    int hashNew = 1;
    Serial.printf("hash is %x\n", hash);
    for( int i = 0 ; i <= _hashEntry ; i++){
        /*
        if(strcmp(hash, hashTable[i]) == 0){
            hashNew = 0; 
            Serial.printf("Not new!\n");
        }
        */
    }
    if(hashNew){
        // add to hash table
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            _hashTable[_hashEntry][i] = hash[i];
        }
        _hashEntry++;
    }
    return hashNew;
}

int Layer1Class::debug_printf(const char* format, ...) {
//   if(DEBUG){
//     int ret;
//     va_list args;
//     va_start(args, format);
//     ret = vfprintf(stderr, format, args);
//     va_end(args);
//     fflush(stderr);
//     return ret;
//   }else{
//     return 0;
//   }
}

uint8_t Layer1Class::hex_digit(char ch){
  if(( '0' <= ch ) && ( ch <= '9' )){
    ch -= '0';
  }else{
    if(( 'a' <= ch ) && ( ch <= 'f' )){
      ch += 10 - 'a';
    }else{
      if(( 'A' <= ch ) && ( ch <= 'F' ) ){
        ch += 10 - 'A';
      }else{
        ch = 16;
      }
    }
  }
  return ch;
}

int Layer1Class::setLocalAddress(char* macString){
  for( int i = 0; i < sizeof(_localAddress)/sizeof(_localAddress[0]); ++i ){
    _localAddress[i]  = hex_digit( macString[2*i] ) << 4;
    _localAddress[i] |= hex_digit( macString[2*i+1] );
  }
  if(_localAddress){
    return 1;
  }else{
    return 0;
  }
}

uint8_t* Layer1Class::localAddress(){
    return _localAddress;
}

int Layer1Class::setNodeID(char* newID){
    _nodeID = newID;
    return 0;
}

int Layer1Class::begin_packet(){
    if(_transmitting == 1){
        // transmission in progress, do not begin packet
        return 0;
    }else{
        // transmission done, begin packet
        return 1;
    }
}

int Layer1Class::parse_metadata(char* data, uint8_t len){
    data[len] = '\0';
    switch(data[0]){
        case 't':
            if(data[1] == '0'){
                _transmitting = 0;
            }else{
                _transmitting = 1;
            }
            break;
        case 'd':
            setTimeDistortion(strtod((data + 1), NULL));
            break;
        default:
            perror("invalid metadata");
    }
    return 0;
}

int Layer1Class::send_packet(char* data, uint8_t len) {
    char packet[258];
    ssize_t written = 0;
    ssize_t ret;
    if(!len) {
        len = strlen(data);
    }
    if(len > 256) {
        fprintf(stderr, "Attempted to send packet larger than 256 bytes\n");
        return -1;
    }
    packet[0] = len;
    memcpy(packet+1, data, len);
    while(written < len) {
        ret = write(STDOUT, (void*) packet, len+1);
        if(ret < 0) {
            return ret;
        }
        written += ret;
    }
    printf("\n");
    fflush(stdout);
    return 0;
}

Layer1Class Layer1;

int print_err(const char* format, ...) {
    // int ret;
    // va_list args;
    // va_start(args, format);
    // ret = vfprintf(stderr, format, args);
    // va_end(args);
    // fflush(stderr);
    // return ret;
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "t:a:n:")) != -1) {
        switch (opt) {
            case 't':
                Layer1.setTimeDistortion(strtod(optarg, NULL));
                break;
            case 'a':
                Layer1.setLocalAddress(optarg);
                break;
            case 'n':
                Layer1.setNodeID(optarg);
                break;
            default:
                perror("Bad args\n");
                return 1;
        }
    }
    char buffer[257];
    struct timeval tv;
    fd_set fds;
    int flags;
    ssize_t ret;
    ssize_t len = 0;
    ssize_t got;
    ssize_t meta = 0;
    Layer1.nsleep(0, 0);
    Serial.printf = &print_err;
    flags = fcntl(STDIN, F_GETFL, 0);
    if(flags == -1) {
        perror("Failed to get socket flags\n");
        return 1;
    }
    flags = flags | O_NONBLOCK;
    if(ret = fcntl(STDIN, F_SETFL, flags)) {
        perror("Failed to set non-blocking mode\n");
        return 1;
    }
    if(ret = setup()) {
        return ret;
    }
    while(1) {
        FD_ZERO(&fds);
        FD_SET(STDIN, &fds);
        tv.tv_sec = to_sleep.tv_sec;
        tv.tv_usec = to_sleep.tv_usec;
        ret = select(STDIN + 1, &fds, NULL, NULL, &tv);
        if(ret < 0) {
          perror("select() failed");
          return 1;
        }
        // WARNING this code assumes that tv is modified to reflect the
        // time not slept during select()
        // which is only true on Linux
        if(tv.tv_sec < 1 && tv.tv_usec < 100) {
            // getting within 100 us is enough.
            to_sleep.tv_sec = 0;
            to_sleep.tv_usec = 0;
        } else {
            to_sleep.tv_sec = tv.tv_sec;
            to_sleep.tv_usec = tv.tv_usec;
        }

        if(ret && FD_ISSET(STDIN, &fds)) {
            if(!meta){
                ret = read(STDIN, &meta, 1);
                if(ret < 0) {
                    perror("failed to read metadata flag");
                    return 1;
                }
            }
            if(!len) {
                // receive the length of the incoming packet
                ret = read(STDIN, &len, 1);
                if(ret < 0) {
                    perror("receiving length of incoming packet failed");
                    return 1;
                }
                got = 0;
            }
            while(got < len) {
                ret = read(STDIN, (void*)(buffer+got), len-got);
                if(ret < 0) {
                    if(errno == EAGAIN) { // we need to wait for more data
                        break;
                    }
                    perror("receiving incoming data failed");
                    return 1;
                }
                got += ret;
            }
            if(got < len) {
                continue;
            }
            if(meta){
                if(ret = Layer1.parse_metadata(buffer, len)){
                    return ret;
                }
            }
            else if(ret = LL2.packetReceived(buffer, len)){
                return ret;
            }
            meta = 0;
            len = 0;
        }
        // if we've slept enough, call loop()
        if(to_sleep.tv_sec == 0 && to_sleep.tv_usec == 0) {
            if(ret = loop()) {
                return ret;
            }
        }
    }
  return 0;
}
#endif
