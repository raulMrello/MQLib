/*
 * test_MQLib.cpp
 *
 *	Unit test file for MQLib module
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "mbed.h"
#include "AppConfig.h"
#include "MQLib.h"


#if ESP_PLATFORM == 1 || (__MBED__ == 1 && defined(ENABLE_TEST_DEBUGGING) && defined(ENABLE_TEST_MQLib))

#if ESP_PLATFORM == 1

#elif __MBED__ == 1 && defined(ENABLE_TEST_DEBUGGING) && defined(ENABLE_TEST_MQLib)
#include "unity_test_runner.h"
#endif

/** required for test execution */
static MQ::PublishCallback s_published_cb;
static MQ::SubscribeCallback s_subscribe_cb;
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len);
static void publishedCb(const char* topic, int32_t result);
static void executePrerequisites();
static const char* s_msg = "Hello";
static bool s_subscription_updated = false;
static uint32_t s_subscription_count = 0;

//------------------------------------------------------------------------------------
//-- SPECIFIC COMPONENTS FOR TESTING -------------------------------------------------
//------------------------------------------------------------------------------------

static const char* _MODULE_ = "[TEST_MQLib]....";
#define _EXPR_	(true)


//------------------------------------------------------------------------------------
//-- TEST CASES ----------------------------------------------------------------------
//------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/**
 * @brief Library creation
 */
TEST_CASE("MQLib creation .......................", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();
}


//---------------------------------------------------------------------------
/**
 * @brief Check simple topics matching:
 * stat/var/0
 * cmd/cfg/par/2
 * set/value/at/var/5
 */
TEST_CASE("Check simple topics matching .........", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();

	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/var/0");
	int32_t res = MQ::MQClient::subscribe("stat/var/0", new MQ::SubscribeCallback(&subscriptionCb));
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to cmd/cfg/par/2");
	res = MQ::MQClient::subscribe("cmd/cfg/par/2", new MQ::SubscribeCallback(&subscriptionCb));
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to set/value/at/var/2");
	res = MQ::MQClient::subscribe("set/value/at/var/2", new MQ::SubscribeCallback(&subscriptionCb));
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// print registered tokens
	const char** tklist;
	uint32_t tkcount;
	MQ::MQClient::getInternalTokenList(tklist, tkcount);
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Registered tokens (should be 10): %d", tkcount);
	TEST_ASSERT_EQUAL(tkcount, 10);

	for(int i=0; i < tkcount; i++){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "%s", *tklist);
		tklist++;
	}

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("stat/var/0", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("cmd/cfg/par/2", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("set/value/at/var/2", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);
}



//---------------------------------------------------------------------------
/**
 * @brief Check simple topics rejections with previous subscriptions:
 * stat/var/1
 * cmd/val/par/2
 * set/param/at/var/5
 */
TEST_CASE("Check simple topics rejections .......", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();
	int32_t res;
	if(!MQ::MQClient::existsTopic("stat/var/0")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/var/0");
		res = MQ::MQClient::subscribe("stat/var/0", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("cmd/cfg/par/2")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to cmd/cfg/par/2");
		res = MQ::MQClient::subscribe("cmd/cfg/par/2", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("set/value/at/var/2")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to set/value/at/var/2");
		res = MQ::MQClient::subscribe("set/value/at/var/2", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("stat/var/1", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("cmd/val/par/2", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("set/param/at/var/5", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);
}



//---------------------------------------------------------------------------
/**
 * @brief Check wildcards
 * stat/var/par/+
 * stat/var/+/+
 * stat/+/par/+
 * stat/+/+/+
 * stat/var/#
 * stat/+/#
 * stat/#
 * #
 */
TEST_CASE("Check simple topics rejections .......", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();
	int32_t res;

	if(!MQ::MQClient::existsTopic("stat/var/par/+")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/var/par/+");
		res = MQ::MQClient::subscribe("stat/var/par/+", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/var/+/+")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/var/+/+");
		res = MQ::MQClient::subscribe("stat/var/+/+", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/+/par/+")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/+/par/+");
		res = MQ::MQClient::subscribe("stat/+/par/+", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/+/+/+")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/+/+/+");
		res = MQ::MQClient::subscribe("stat/+/+/+", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/var/#")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/var/#");
		res = MQ::MQClient::subscribe("stat/var/#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/+/#")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/+/#");
		res = MQ::MQClient::subscribe("stat/+/#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("stat/#")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to stat/#");
		res = MQ::MQClient::subscribe("stat/#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	if(!MQ::MQClient::existsTopic("#")){
		DEBUG_TRACE_D(_EXPR_, _MODULE_, "Suscription to #");
		res = MQ::MQClient::subscribe("#", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	}

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("stat/var/1", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 4);

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("cmd/val/par/2", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 1);

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("stat/value/data/3", (void*)s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 4);

}

//---------------------------------------------------------------------------
/**
 * @brief Library creation
 */

static void processBridge1(const char* topic, void* data, uint16_t datasize, MQ::PublishCallback* publisher){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Procesando bridging del topic '%s' ---> 'topic/dato1'", topic);
	uint32_t* udata = (uint32_t*) data;
	uint8_t u8data = (uint8_t)*udata;
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "publicando en 'topic/dato1' el dato %x", u8data);
	MQ::MQClient::republish("topic/dato1", &u8data, sizeof(uint8_t), publisher);
}

static void processBridge2(const char* topic, void* data, uint16_t datasize, MQ::PublishCallback* publisher){
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Procesando bridging del topic '%s' ---> 'topic/dato2'", topic);
	uint32_t* udata = (uint32_t*) data;
	uint8_t u8data = (uint8_t)(*udata >> 8);
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "publicando en 'topic/dato2' el dato %x", u8data);
	MQ::MQClient::republish("topic/dato2", &u8data, sizeof(uint8_t), publisher);
}
static MQ::BridgeCallback bc1;
static MQ::BridgeCallback bc2;

TEST_CASE("MQLib BRIDGE CREATION ................", "[MQLib]") {
	s_subscribe_cb = callback(&subscriptionCb);
	bc1 = callback(&processBridge1);
	bc2 = callback(&processBridge2);
	TEST_ASSERT_EQUAL(MQ::MQClient::subscribe("topic/#", &s_subscribe_cb), MQ::SUCCESS);
	TEST_ASSERT_EQUAL(MQ::MQClient::addBridge("topic/bridge", &bc1), MQ::SUCCESS);
	TEST_ASSERT_EQUAL(MQ::MQClient::addBridge("topic/bridge", &bc2), MQ::SUCCESS);
}

TEST_CASE("MQLib BRIDGE TEST ....................", "[MQLib]") {
	uint32_t u32 = 0x12345678;
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "publicando en 'topic/bridge' el dato 0x12345678");
	TEST_ASSERT_EQUAL(MQ::MQClient::publish("topic/bridge", &u32, sizeof(uint32_t), &s_published_cb), MQ::SUCCESS);
}

TEST_CASE("MQLib BRIDGE1 REMOVE .................", "[MQLib]") {
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Eliminando bridge 'topic/dato1'");
	TEST_ASSERT_EQUAL(MQ::MQClient::removeBridge("topic/bridge", &bc1), MQ::SUCCESS);
}

TEST_CASE("MQLib BRIDGE2 REMOVE .................", "[MQLib]") {
	DEBUG_TRACE_D(_EXPR_, _MODULE_, "Eliminando bridge 'topic/dato2'");
	TEST_ASSERT_EQUAL(MQ::MQClient::removeBridge("topic/bridge", &bc2), MQ::SUCCESS);
}

//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------


/** Prerequisites execution control flag */
static bool s_executed_prerequisites = false;

//------------------------------------------------------------------------------------
static void publishedCb(const char* topic, int32_t result){
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Published on topic=<%s> with result=%d", topic, result);
}

//------------------------------------------------------------------------------------
static void subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Recibido topic %s con mensaje de %d bytes", topic, msg_len);
	s_subscription_updated = true;
	s_subscription_count++;
}

//------------------------------------------------------------------------------------
static void executePrerequisites(){
	if(!s_executed_prerequisites){
		// inicia mqlib
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init MQLib...");
		int32_t res = MQ::MQBroker::start(64);
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// espera a que esté disponible
		while(!MQ::MQBroker::ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "MQLib OK!");

		// registra un manejador de publicaciones común
		s_published_cb = callback(&publishedCb);

		// marca flag de prerequisitos completado
		s_executed_prerequisites = true;
	}
}

#if __MBED__ == 1
void firmwareStart(bool wait_forever){
	esp_log_level_set(_MODULE_, ESP_LOG_DEBUG);
	DEBUG_TRACE_I(_EXPR_, _MODULE_, "Inicio del programa");
	Heap::setDebugLevel(ESP_LOG_ERROR);
	unity_run_menu();
}
#endif

#endif
