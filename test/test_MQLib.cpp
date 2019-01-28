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
 * @brief Check if topic stat/+/+/+ rejects updates on stat/all/vars
 * MQLib
 */
TEST_CASE("Check length rejection................", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();

	// clear result flag
	s_subscription_updated = false;

	// publish
	MQ::ErrorResult res = MQ::MQClient::publish("stat/all/vars", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// check result: subscription not handled
	TEST_ASSERT_FALSE(s_subscription_updated);
}

//---------------------------------------------------------------------------
/**
 * @brief Check if topic stat/+/+/+ accepts updates on stat/all/vars/done
 * MQLib
 */
TEST_CASE("Check length acceptance...............", "[MQLib]") {

	// Execute test pre-requisites
	executePrerequisites();

	// clear result flag
	s_subscription_updated = false;

	// publish
	MQ::ErrorResult res = MQ::MQClient::publish("stat/all/vars/done", s_msg, strlen(s_msg)+1, &s_published_cb);
	TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

	// check result: subscription not handled
	TEST_ASSERT_TRUE(s_subscription_updated);
}



//------------------------------------------------------------------------------------
//-- PREREQUISITES -------------------------------------------------------------------
//------------------------------------------------------------------------------------

/** Tokens MQLib requeridos para este test */
static MQ::Token token_list[] = {
	P2P_PRODUCT_FAMILY,
	P2P_PRODUCT_TYPE,
	P2P_PRODUCT_SERIAL,
	"stat",
	"all",
	"vars",
	"done",
};
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
}

//------------------------------------------------------------------------------------
static void executePrerequisites(){
	if(!s_executed_prerequisites){
		// inicia mqlib
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Init MQLib...");
		MQ::ErrorResult res = MQ::MQBroker::start(token_list, SizeOfArray(token_list), 64, P2P_PRODUCT_FAMILY, P2P_PRODUCT_TYPE, true);
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// espera a que esté disponible
		while(!MQ::MQBroker::ready()){
			Thread::wait(100);
		}
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "MQLib OK!");

		// registra un manejador de publicaciones común
		s_published_cb = callback(&publishedCb);

		// set subscription: stat/+/+/+
		DEBUG_TRACE_I(_EXPR_, _MODULE_, "Suscription to stat/+/+/+");
		res = MQ::MQClient::subscribe("stat/+/+/+", new MQ::SubscribeCallback(&subscriptionCb));
		TEST_ASSERT_EQUAL(res, MQ::SUCCESS);

		// marca flag de prerequisitos completado
		s_executed_prerequisites = true;
	}
}


