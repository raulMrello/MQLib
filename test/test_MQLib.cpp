/*
 * test_MQLib.cpp
 *
 *	Unit test file for MQLib module
 */


//------------------------------------------------------------------------------------
//-- TEST HEADERS --------------------------------------------------------------------
//------------------------------------------------------------------------------------

#include "unity.h"
#include "MQLib.h"
#include <esp_task_wdt.h>

//------------------------------------------------------------------------------------
//-- REQUIRED HEADERS & COMPONENTS FOR TESTING ---------------------------------------
//------------------------------------------------------------------------------------

#include "AppConfig.h"

/** required for test execution */
static MQ::PublishCallback s_published_cb;
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
	MQ::ErrorResult res = MQ::MQClient::subscribe("stat/var/0", new MQ::SubscribeCallback(&subscriptionCb));
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
	res = MQ::MQClient::publish("stat/var/0", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("cmd/cfg/par/2", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("set/value/at/var/2", s_msg, strlen(s_msg)+1, &s_published_cb);
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
	MQ::ErrorResult res;
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
	res = MQ::MQClient::publish("stat/var/1", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("cmd/val/par/2", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);

	// clear result flag
	s_subscription_updated = false;
	// publish
	res = MQ::MQClient::publish("set/param/at/var/5", s_msg, strlen(s_msg)+1, &s_published_cb);
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
	MQ::ErrorResult res;

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

	// watchdog reset for avoiding CPU starvation
	esp_task_wdt_reset();

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

	// watchdog reset for avoiding CPU starvation
	esp_task_wdt_reset();

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("stat/var/1", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 4);

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("cmd/val/par/2", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 1);

	// clear result flag
	s_subscription_updated = false;
	s_subscription_count = 0;
	// publish
	res = MQ::MQClient::publish("stat/value/data/3", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);
	TEST_ASSERT_TRUE(s_subscription_updated);
	TEST_ASSERT_EQUAL(s_subscription_count, 4);

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
		MQ::ErrorResult res = MQ::MQBroker::start(64, true);
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


