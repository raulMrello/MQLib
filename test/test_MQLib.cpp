#include "mbed.h"
#include "MQLib.h"


// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresi�n de trazas de depuraci�n */
#define DEBUG_TRACE			printf


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************


/** Callbacks MQLib */
static MQ::PublishCallback publ_cb;

/** Topics de publicaci�n, suscripci�n */
static const char* sub_topic = "/move/stop";
static char* txt_msg = "Hello";
static void* msg = (void*) txt_msg;
static uint32_t msg_len = strlen(txt_msg)+1;


// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************

static void publCb(const char* name, int32_t){
}


//------------------------------------------------------------------------------------
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
    if(MQ::MQClient::isTopicToken(topic, "/A")){
        DEBUG_TRACE("\r\nTopic:%s, token:/A msg:%s\r\n", topic, (char*)msg);
        MQ::MQClient::publish("config/start/AB", msg, msg_len, &publ_cb);
        return;
    }
    if(MQ::MQClient::isTopicToken(topic, "/AB")){
        DEBUG_TRACE("\r\nTopic:%s, token:/AB msg:%s\r\n", topic, (char*)msg);
        MQ::MQClient::publish("config/start/ABC", msg, msg_len, &publ_cb);
        return;
    }
    if(MQ::MQClient::isTopicToken(topic, "/ABC")){
        DEBUG_TRACE("\r\nTopic:%s, token:/ABC msg:%s\r\n", topic, (char*)msg);
        MQ::MQClient::publish("config/start/ABCD", msg, msg_len, &publ_cb);
        return;
    }
    if(MQ::MQClient::isTopicToken(topic, "/ABCD")){
        DEBUG_TRACE("\r\nTopic:%s, token:/ABCD msg:%s\r\n", topic, (char*)msg);
        return;
    }
}



//------------------------------------------------------------------------------------
void test_MQLib(){
            
    publ_cb = callback(&publCb);
    
    DEBUG_TRACE("\r\nIniciando test_MQLib...\r\n");      

        
    // --------------------------------------
    // Arranca el broker con la siguiente configuraci�n:
    //  - Lista de tokens predefinida
    //  - N�mero m�ximo de caracteres para los topics: 64 caracteres incluyendo fin de cadena '\0'
    //  - Espera a que est� operativo
    MQ::MQBroker::start(64, true);
    while(!MQ::MQBroker::ready()){
        Thread::wait(1);
    }
	
    
    DEBUG_TRACE("\r\nSuscripci�n a eventos move/stop/# ...");
    MQ::MQClient::subscribe(sub_topic, new MQ::SubscribeCallback(&subscriptionCb));
    DEBUG_TRACE("OK!\r\n");
    
    // --------------------------------------
    // Arranca el test
    DEBUG_TRACE("\r\n...................INICIO DEL TEST.........................\r\n");  
    DEBUG_TRACE("\r\nPublish on config/start/A \r\n");    
    MQ::MQClient::publish("config/start/A", msg, msg_len, &publ_cb);

}

