#ifdef USE_SX1262
#include <LoRa1262.h>
#else
#include <LoRa.h>
#endif
#include <Layer1.h>
#include <LoRaLayer2.h>
#ifdef LORA 


Layer1Class::Layer1Class() :

    _hashTable(),
    _hashEntry(0),
    _loraInitialized(0),

    _csPin(L1_DEFAULT_CS_PIN),
    _resetPin(L1_DEFAULT_RESET_PIN),
    _DIOPin(L1_DEFAULT_DIO0_PIN),
    _spiFrequency(100E3),
    _loraFrequency(915E6),
    _spreadingFactor(9),
    _txPower(17)

{

}

int Layer1Class::debug_printf(const char* format, ...) {

    // if(DEBUG){
    //     int ret;
    //     va_list args;
    //     va_start(args, format);
    //     ret = vfprintf(stderr, format, args);
    //     va_end(args);
    //     fflush(stderr);
    //     return ret;
    // }else{
    //     return 0;
    // }
}

/*
uint8_t* Layer1Class::charToHex(char* charString){
    int hexLength = strlen(charString)/2;
    uint8_t hexString[hexLength];
    for( int i = 0 ; i < hexLength ; ++i ){
        hexString[i]  = hex_digit(charString[2*i]) << 4;
        hexString[i] |= hex_digit(charString[2*i+1]);
    }
    uint8_t *hexPtr = hexString;
    return hexPtr;
}
*/

int Layer1Class::getTime(){
    return millis();
}

int Layer1Class::isHashNew(char incoming[SHA1_LENGTH]){
    int hashNew = 1;
    for( int i = 0 ; i <= _hashEntry ; i++){
        if(strcmp(incoming, (char*) _hashTable[i]) == 0){
            hashNew = 0; 
        }
    }
    if( hashNew ){
        Serial.printf("New message received");
        Serial.printf("\r\n");
        for( int i = 0 ; i < SHA1_LENGTH ; i++){
            _hashTable[_hashEntry][i] = incoming[i];
        }
        _hashEntry++;
    }
    return hashNew;
}

void Layer1Class::onReceive(int packetSize) {

    if (packetSize == 0) return;          // if there's no packet, return
    char incoming[PACKET_LENGTH];                 // payload of packet
    int incomingLength = 0;
    //Serial.printf("Receiving: ");
    while (LoRa.available()) { 
        incoming[incomingLength] = (char)LoRa.read(); 
        //Serial.printf("%02x", incoming[incomingLength]);
        incomingLength++;
    }
    //Serial.printf("\r\n");
    LL2.packetReceived(incoming, incomingLength);
}

int Layer1Class::loraInitialized(){
    return _loraInitialized;
}

int Layer1Class::loraCSPin(){
    return _csPin;
}

int Layer1Class::resetPin(){
    return _resetPin;
}

int Layer1Class::DIOPin(){
    return _DIOPin;
}

void Layer1Class::setPins(int cs, int reset, int dio){
    _csPin = cs;
    _resetPin = reset;
    _DIOPin = dio;
}

void Layer1Class::setSPIFrequency(uint32_t frequency){
    _spiFrequency = frequency;
}

void Layer1Class::setLoRaFrequency(uint32_t frequency){
    _loraFrequency = frequency;
}

void Layer1Class::setSpreadingFactor(uint8_t spreadingFactor){
    _spreadingFactor = spreadingFactor;
}

void Layer1Class::setTxPower(int txPower){
    _txPower = txPower;
}

int Layer1Class::init(){ // maybe this should take the pins and spreading factor as inputs?

    LoRa.setPins(_csPin, _resetPin, _DIOPin); // set CS, reset, DIO pin
    LoRa.setSPIFrequency(_spiFrequency);
    LoRa.setTxPower(_txPower);

    if (!LoRa.begin(_loraFrequency)) {             // initialize ratio at 915 MHz
        return _loraInitialized;
    }

    LoRa.setSpreadingFactor(_spreadingFactor);           // ranges from 6-12,default 7 see API docs
    LoRa.onReceive(onReceive);
    LoRa.receive();

    _loraInitialized = 1;
    return _loraInitialized;
}

int Layer1Class::send_packet(char* data, int len){

    //Serial.printf("Sending: ");
    if(LoRa.beginPacket()){
        for( int i = 0 ; i < len ; i++){
            LoRa.write(data[i]);
            //Serial.printf("%02x", data[i]);
        }
        //Serial.printf("\r\n");
        LoRa.endPacket(1);
        LoRa.receive();
    }
}

Layer1Class Layer1;

#endif
