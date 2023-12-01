/*
 * MQLib.h
 *
 *  Versi�n: 12 Apr 2018
 *  Author: raulMrello
 *
 *	-------------------------------------------------------------------------------------------------------------------
 *
 *	Changelog:
 *	- Cambia la descripci�n de <name> en <struct Topic> para que pase de un <const char*> a un <char*> y que en el servicio
 *	MQBroker::subscribeReq se reserve espacio para copiar el topic que se desea, de esa forma no es necesario prepararlo
 *	externamente y puede ser liberado insitu por la propia librer�a MQLib.
 *  - @13Abr2018.001 Cambio WildcardScope por WildcardScopeDev, WildcardScopeGroup y AddrField = 2 (antes 1)
 *  - @15Mar2018.001 A�ado clase MQBridge para crear redirecciones de forma c�moda
 *  - @06Mar2018.001 A�ado lista de operaciones pendientes, as� como servicios privados 'addPendingRequest' y
 *  				 'processPendingRequests'. Las esperas en el mutex no son bloqueantes. Esto permite que los suscriptores
 *  				 puedan utilizar la publicaci�n y/o suscripci�n sin quedarse bloqueados de forma permanente.
 *  - @12Apr2018.001 A�ado servicio MQ::MQClient::isTopicRoot
 *  - @05Mar2018.001 verifico wildcards en tokens de cualquier posici�n (soluciona bug)
 *  - @21Feb2018.001 en List.hpp sustituyo por getNextItem para asegurar que _search se posiciona correctamente
 *	- @14Feb2018.001: 'name' cambia de const char* a char*
 *	- @14Feb2018.002: se reserva espacio para el nombre del topic
 *	- @14Feb2018.003: elimina un topic de la lista si se queda sin suscriptores.
 *
 *	-------------------------------------------------------------------------------------------------------------------
 *
 *  MQLib es una librer�a que proporciona capacidades de publicaci�n-suscripci�n de forma pasiva, sin necesidad de
 *	utilizar un thread dedicado y siempre corriendo en el contexto del publicador.
 *
 *	Consta de dos tipos de clases est�ticas: MQBroker y MQClient.
 *
 *	MQBroker: se encarga de gestionar la lista de suscriptores y el paso de mensajes desde los publicadores a �stos.
 *	 Para ello necesita mantener una lista de topics y los suscriptores a cada uno de ellos.
 *
 *	MQClient: se encarga de hacer llegar los mensajes publicados al broker, de forma que �ste los procese como corresponda.
 *
 *  Los Topics se identifican mediante una cadena de texto separada por tokens '/' que indican el nivel de profundidad
 *  del recurso al que se accede (ej: 'aaa/bbb/ccc/ddd' tiene 4 niveles de profundidad: aaa, bbb, ccc, ddd).
 *
 *  Adem�s cada nivel se identifica mediante un identificador �nico. As� para el ejemplo anterior, el token 'aaa' tiene un
 *  identificador, 'bbb' tiene otro, y lo mismo para 'ccc' y 'ddd'.
 *
 *  Esta librer�a est� limitada con unos par�metros por defecto, de forma que encajen en una gran variedad de aplicaciones
 *  sin necesidad de tocar esos par�metros. Sin embargo, podr�an modificarse adapt�ndola a casos especiales. Las limitaciones
 *  son �stas:
 *
 *  Identificador de topic: Variable entera 64-bit (uint64_t)
 *
 *  Relaci�n entre "Identificador de token" vs "Niveles de profundidad"
 *     TokenId_size vs  NivelesDeProfundidad   
 *      8bit_token  -> 8 niveles (m�ximo de 256 tokens)
 *      9bit_token  -> 7 niveles (m�ximo de 512 tokens)
 *      10bit_token -> 6 niveles (m�ximo de 1024 tokens)
 *      13bit_token -> 5 niveles (m�ximo de 8192 tokens)
 *      16bit_token -> 4 niveles (m�ximo de 65536 tokens)
 *
 *  La configuraci�n se selecciona mediante el par�metro MQ_CONFIG_VALUE
 *
 *  Es posible publicar y suscribirse a topics dedicados relativos a un �mbito concreto que se sale de la norma de identificaci�n
 *  de los topics. El wildcard es '@' de esta forma se genera un �mbito "scope" al que se dirige el mensaje. Dicho scope 
 *  est� formado por un valor uint32_t.
 *
 *  Por ejemplo para dirigir mensajes concretos al grupo 3, el topic dedicado ser� de la forma "@group/3/..." O para mensajes
 *  a un dispositivo, por ejemplo el 8976: "@dev/8976/...". De igual forma, se podr�n realizar suscripciones, por ejemplo a
 *  todos los topics con destino el grupo 78: "@group/78/#".
 *
 *  En caso de no utilizar el wildcard "scope", su valor ser� 0 y se considerar� un topic general.
 *
 */

#ifndef MQLIB_H_
#define MQLIB_H_


//------------------------------------------------------------------------------------
#include "mbed.h"
#include "List.h"
#include "Heap.h"
#include <list>
#include <vector>
#include <map>


//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

namespace MQ{	
    

/** Longitud por defecto para el nombre de los topics
 */
static const uint8_t DefaultMaxTopicNameLength = 64;

    
/** @struct MQ::MAX_TOKEN_LEVEL
 *  @brief Tipo definido para definir la profundidad m�xima de los topics
 */
static const uint8_t MAX_TOKEN_LEVEL = 10;
   
    
/** @struct MQ::token_t
 *  @brief Tipo definido para definir el valor de un token
 */
typedef uint8_t token_t;
    
    
/** @struct MQ::Token
 *  @brief Tipo definido para la definici�n de tokens
 */
typedef const char* Token;
    
    
/** @type MQ::SubscribeCallback
 *  @brief Tipo definido para las callbacs de suscripci�n
 */
typedef Callback<void(const char* name, void*, uint16_t)> SubscribeCallback;

/** @type MQ::PublishCallback
 *  @brief Tipo definido para las callbacs de publicaci�n
 */
 typedef Callback<void(const char* name, int32_t)> PublishCallback;

 /** @type MQ::SubscribeCallback
  *  @brief Tipo definido para las callbacs de suscripci�n
  */
 typedef Callback<void(const char* name, void*, uint16_t, PublishCallback*)> BridgeCallback;

/** @enum ErrorResult
 *  @brief Resultados de error generados por la librer�a
 */
enum ErrorResult{
	SUCCESS = 0,          	///< Operaci�n correcta
	NULL_POINTER,           ///< Fallo por Puntero nulo
	DEINIT,                 ///< Fallo por no-inicializaci�n
	OUT_OF_MEMORY,          ///< Fallo por falta de memoria
	EXISTS,           		///< Fallo por objeto existente
	NOT_FOUND,        		///< Fallo por objeto no existente
    OUT_OF_BOUNDS,          ///< Fallo por exceso de tama�o
    LOCK_TIMEOUT,			///< Fallo por timeout en el lock
};
	
    
/** @struct MQ::topic_t
 *  @brief Tipo definido para definir el identificador de un topic
 */
struct __packed topic_t{
    uint8_t tk[MQ::MAX_TOKEN_LEVEL+1];
};



/** @struct Topic
 *  @brief Estructura asociada los topics, formada por un nombre y una lista de suscriptores
 *  	   @14Feb2018.001: 'name' cambia de const char* a char*
 */
struct Topic{
    MQ::topic_t id;              					/// Identificador del topic
    char* name;                               		/// Nombre del name asociado a este nivel
	List<MQ::SubscribeCallback > *subscriber_list; 	/// Lista de suscriptores
};


	
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------



class MQBroker {
public:	    

    /** @fn start
     *  @brief Inicializa el broker MQ estableciendo el n�mero m�ximo de caracteres en los topics
     *         Este constructor se utiliza cuando no se proporciona una lista de tokens externa, sino
     *         que se crea conforme se realizan las diferentes suscripciones a topics.
     *  @param max_len_of_name N�mero de caracteres m�ximo que puede tener un topic (incluyendo '\0' final)
     *  @param wildcard_scope_dev Token para identificador num�rico de dispositivo
     *  @param wildcard_scope_group Token para identificador num�rico de grupo
     *  @param defdbg Flag para activar las trazas de depuraci�n por defecto
     *  @return C�digo de error
     */
    static int32_t start(uint8_t max_len_of_name, bool defdbg = false){
    	int32_t rc = SUCCESS;
    	_pub_count = 0;
    	_mutex.lock();
        // ajusto par�metros por defecto 
    	setLoggingLevel((defdbg)? ESP_LOG_DEBUG : ESP_LOG_INFO);
    	_defdbg = true;
        _max_name_len = max_len_of_name-1;
        DEBUG_TRACE_I(_defdbg,"[MQLib].........", "Iniciando Broker...");
		_tokenlist_internal = true;
		_token_provider_count = WildcardCOUNT;
		_token_provider = (const char**)Heap::memAlloc(DefaultMaxNumTokenEntries * sizeof(const char*));
		if(!_token_provider){
			rc = NULL_POINTER; goto __start_exit;
		}
        
        // creo lista de solicitudes pendientes
        _pending_list = new List<PendingRequest_t>();
        MBED_ASSERT(_pending_list);

        // si hay un n�mero de tokens mayor que el tama�o que lo puede alojar, devuelve error:
        // ej: token_count = 500 con token_t = uint8_t, que s�lo puede codificar hasta 256 valores.
        if(((DefaultMaxNumTokenEntries+WildcardCOUNT) >> (8*sizeof(MQ::token_t))) > 1){
            rc = OUT_OF_BOUNDS; goto __start_exit;
        }
        
        // si no hay lista inicial, se crea...
        if(!_topic_list){

            _topic_list = new List<MQ::Topic>();
			if(!_topic_list){
				rc = OUT_OF_MEMORY; goto __start_exit;
			}
			_topic_list->setLimit(DefaultMaxNumTopics);
			rc = SUCCESS; goto __start_exit;
        }
		rc = EXISTS; goto __start_exit;

__start_exit:
	_mutex.unlock();
	return rc;
    }

    static void setLoggingLevel(esp_log_level_t level){
    	esp_log_level_set("[MQLib].........", level);
    }


    /** @fn ready
     *  @brief Chequea si el broker est� listo para ser usado
	 *	@return True:listo, False:pendiente
     */
    static bool ready() {
		return ((_topic_list)? true : false);
	}

    
    /** @fn subscribeReq
     *  @brief Recibe una solicitud de suscripci�n a un topic
     *  @param name Nombre del topic
     *  @param subscriber Manejador de las actualizaciones del topic
     *  @param use_lock Flag para utilizar el bloqueo por mutex
     *  @return Resultado
     */
    static int32_t subscribeReq(const char* name, MQ::SubscribeCallback *subscriber, bool use_lock = true){
        int32_t err;
        if(!_topic_list){
            return DEINIT;
        }
        // si el nombre excede el tama�o m�ximo, no lo permite
        if(strlen(name) > _max_name_len){
            return OUT_OF_BOUNDS;
        }

        DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Iniciando suscripci�n a [%s]", name);

        // Inicia la b�squeda del topic para ver si ya existe
        if(use_lock){
        	osStatus oss;
			if((oss = _mutex.lock(DefaultMutexTimeout)) != osOK){
				DEBUG_TRACE_E(true,"[MQLib].........", "ERR_SUBSCRIBE [%d] en topic %s", oss, name);
				return LOCK_TIMEOUT;
				//return addPendingRequest(ReqSubscribe, name, NULL, 0, NULL, subscriber);
			}
        }

        MQ::Topic * topic = findTopicByName(name);		
		// si lo encuentra...
        if(topic){
			// Chequea si el suscriptor ya existe...
			MQ::SubscribeCallback *sbc = topic->subscriber_list->searchItem(subscriber);
			// si no existe, lo a�ade
			if(!sbc){
				err = topic->subscriber_list->addItem(subscriber);
				goto _subscribe_exit;
			}
			// si existe, devuelve el error
			DEBUG_TRACE_W(_defdbg,"[MQLib].........", "ERR_SUBSC. El suscriptor ya existe");
			err = EXISTS; goto _subscribe_exit;
        }
		
		// si el topic no existe:
        DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Creando topic [%s]", name);
        // si la lista de tokens es automantenida, crea los ids de los tokens no existentes
        if(_tokenlist_internal){
            if(!generateTokens(name)){
                err = OUT_OF_MEMORY; goto _subscribe_exit;
            }
        }
        // lo crea reservarvando espacio para el topic
        topic = (MQ::Topic*)Heap::memAlloc(sizeof(MQ::Topic));
        if(!topic){
            err = OUT_OF_MEMORY; goto _subscribe_exit;
        }
        
        // se fijan los par�metros del token (delimitadores, id, nombre)

        //@14Feb2018.002: se reserva espacio para el nombre del topic
        topic->name = (char*)Heap::memAlloc(strlen(name)+1);
        if(!topic->name){
        	err = OUT_OF_MEMORY; goto _subscribe_exit;
        }
        strcpy(topic->name, name);
        createTopicId(&topic->id, name);

        // se crea la lista de suscriptores
        topic->subscriber_list = new List<MQ::SubscribeCallback>();
        if(!topic->subscriber_list){
            err = OUT_OF_MEMORY; goto _subscribe_exit;
        }
        
        // y se a�ade el suscriptor
        if(topic->subscriber_list->addItem(subscriber) != SUCCESS){
            err = OUT_OF_MEMORY; goto _subscribe_exit;
        }
        
        // se inserta en el �rbol de topics
        err = _topic_list->addItem(topic);

_subscribe_exit:
		if(use_lock){
			_mutex.unlock();
//			processPendingRequests();
		}
        return err;
    }

	
    /** @fn unsubscribeReq
     *  @brief Recibe una solicitud de cancelaci�n de suscripci�n a un topic
     *  @param name Nombre del topic
     *  @param subscriber Suscriptor a eliminar de la lista de suscripci�n
     *  @param use_lock Flag para utilizar el bloqueo por mutex
     *  @return Resultado
     */
    static int32_t unsubscribeReq (const char* name, MQ::SubscribeCallback *subscriber, bool use_lock = true){
		int32_t err;
		if(!_topic_list){
            return DEINIT;
        }
        // si el nombre excede el tama�o m�ximo, no lo permite
        if(strlen(name) > _max_name_len){
            return OUT_OF_BOUNDS;
        }


        // Inicia la b�squeda del topic para ver si ya existe
        if(use_lock){
        	osStatus oss;
			if((oss = _mutex.lock(DefaultMutexTimeout)) != osOK){
				DEBUG_TRACE_E(true,"[MQLib].........", "ERR_UNSUBSCRIBE [%d] en topic %s", oss, name);
				return LOCK_TIMEOUT;
				//return addPendingRequest(ReqUnsubscribe, name, NULL, 0, NULL, subscriber);
			}
        }

        // precargo posible error
        err = NOT_FOUND;

        MQ::Topic * topic = findTopicByName(name);
        if(topic){
        	MQ::SubscribeCallback *sbc = topic->subscriber_list->searchItem(subscriber);
			if(sbc){
				err = topic->subscriber_list->removeItem(sbc);
				//@14Feb2018.003: elimina un topic de la lista si se queda sin suscriptores.
				if(topic->subscriber_list->getItemCount() == 0){
					Heap::memFree(topic->name);
					_topic_list->removeItem(topic);
					Heap::memFree(topic);
				}
			}
        }

		if(use_lock){
			_mutex.unlock();
//			processPendingRequests();
		}
		return err;
    }
	
	
    /** @fn publishReq
     *  @brief Recibe una solicitud de publicaci�n a un topic
     *  @param name Nombre del topic
     *  @param data Mensaje
     *  @param datasize Tama�o del mensaje
     *  @param publisher Callback de notificaci�n de la publicaci�n
     *  @param use_lock Flag para utilizar el bloqueo por mutex
	 *	@return Resultado
     */
    static int32_t publishReq (const char* name, void *data, uint32_t datasize, MQ::PublishCallback *publisher, bool use_lock = true){
        static int errors=0;
    	if(!_topic_list){
            return DEINIT;
        }
        // si el nombre excede el tama�o m�ximo, no lo permite
        if(strlen(name) > _max_name_len){
            return OUT_OF_BOUNDS;
        }

        // Inicia la b�squeda del topic para ver si ya existe
        if(use_lock){
        	osStatus oss;
			if((oss = _mutex.lock(DefaultMutexTimeout)) != osOK){
                if(++errors > 3){
				#if ESP_PLATFORM == 1
                SaveResetReasonKey("MQLibPublish");
				esp_restart();
				#elif __MBED__ == 1
				NVIC_SystemReset();
				#endif
                }
				DEBUG_TRACE_E(true,"[MQLib].........", "ERR_PUBLISH id=[%d] err=[%d] en topic %s", _pub_count++, oss, name);
				return LOCK_TIMEOUT;
				//return addPendingRequest(ReqPublish, name, data, datasize, publisher, NULL);
			}
        }

        DEBUG_TRACE_D(true, "[MQLib].........", "Publicacion [%d] en topic  '%s'", _pub_count++, name);

        // si la lista de tokens es automantenida, crea los ids de los tokens no existentes
        if(_tokenlist_internal){
            if(!generateTokens(name)){
                if(use_lock){
        			_mutex.unlock();
//        			processPendingRequests();
        		}
        		return OUT_OF_MEMORY;
            }
        }

		// obtiene el identificador del topic a publicar
        MQ::topic_t topic_id;
        createTopicId(&topic_id, name);
        
        // copia el mensaje a enviar por si sufre modificaciones, no alterar el origen
        char* mem_data = (char*)Heap::memAlloc(datasize);
        MBED_ASSERT(mem_data);
        
        DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Buscando topic '%s' en la lista", name);
        MQ::Topic* topic = _topic_list->getFirstItem();
        bool notify_subscriber = false;
        while(topic){
        	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Comparando topic '%s' con '%s'", name, topic->name);
            // comprueba si el id coincide o si no se usa (=0)
            if(matchIds(&topic->id, &topic_id)){
            	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Topic '%s' encontrado. Buscando suscriptores...", name);
                // si coinciden, se invoca a todos los suscriptores                
                MQ::SubscribeCallback *sbc = topic->subscriber_list->getFirstItem();
                while(sbc){
                    // restaura el mensaje por si hubiera sufrido modificaciones en alg�n suscriptor
                    memcpy(mem_data, data, datasize);
                    DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Notificando topic update de '%s' al suscriptor %x", name, (uint32_t)sbc);
                    notify_subscriber = true;
                    sbc->call(name, mem_data, datasize);
                    sbc = topic->subscriber_list->getNextItem();
                }                    
            }
            topic = _topic_list->getNextItem();
        }
        publisher->call(name, (notify_subscriber)? SUCCESS : NOT_FOUND);
        Heap::memFree(mem_data);
        DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Fin de la publicaci�n del topic '%s'", name);

        if(use_lock){
			_mutex.unlock();
//			processPendingRequests();
		}
        errors = 0;
		return SUCCESS;
    }

    
    /** @fn getTopicIdReq 
     *  @brief Obtiene el identificador del topic dado su nombre
     *  @param id Recibe el Identificador del topic or (0) si no existe
     *  @param name Nombre del topic
     */
    static void getTopicIdReq(MQ::topic_t* id, const char* name){        
        createTopicId(id, name);
    }    

    
    /** @fn getTopicNameReq 
     *  @brief Obtiene el nombre de un topic dado su id y su scope
     *  @param name Recibe el nombre asociado al id
     *  @param len Tama�o m�ximo aceptado para el nombre
     *  @param id Identificador del topic
     */
    static void getTopicNameReq(char* name, uint8_t len, MQ::topic_t* id){
        strcpy(name, "");

        // recorre campo a campo verificando los tokens
        for(int i=0;i<MQ::MAX_TOKEN_LEVEL;i++){
            uint32_t idex;
			// obtiene el token_id
			idex = id->tk[i];
			// si ha llegado al final del topic, termina
			if(idex == WildcardNotUsed){
				// una vez finalizado, debe borrar el �ltimo caracter '/' insertado
				name[strlen(name)-1] = 0;
				return;
			}
			// si son wildcards, los escribe
			else if(idex == WildcardAny){
				strcat(name, "+/");
			}
			else if(idex == WildcardAll){
				strcat(name, "#/");
			}
			// en otro caso, escribe el token correspondiente
			else{
				strcat(name, _token_provider[idex]);
				strcat(name, "/");
			}
        }
        // una vez finalizado, debe borrar el �ltimo caracter '/' insertado
        name[strlen(name)-1] = 0;
    }        

    
    /** @fn getMaxTopicLenReq 
     *  @brief Obtiene el n�mero m�ximo de caracteres que puede tener un topic
     *  @return N�mero de caracteres, incluyendo el '\0' final.
     */
    static uint8_t getMaxTopicLenReq(){
        return _max_name_len;
    }


    /** @fn getInternalTokenListReq
     *  @brief Obtiene la lista de tokens interna
     *  @param tklist Lista de tokens
     *  @param tkcount N�mero de tokens en la lista
     */
    static void getInternalTokenListReq(const char** &tklist, uint32_t &tkcount){
        tklist = _token_provider;
        tkcount = _token_provider_count - WildcardCOUNT;
    }


    /** @fn existsTopicReq
     *  @brief Chequea si un topic existe
     *  @param name Nombre del topic a chequear
     */
    static bool existsTopicReq(const char* name){
    	MQ::Topic* topic = _topic_list->getFirstItem();
    	while(topic){
    		if(strcmp(name, topic->name) == 0){
    			return true;
    		}
    		topic = _topic_list->getNextItem();
    	}
    	return false;
    }

    /** M�ximo tiempo de espera en el mutex antes de crear solicitud pendiente */
    static const uint32_t DefaultMutexTimeout = 3000;


private:
	
    /** Contador de publicaciones */
    static uint32_t _pub_count;

    /** Identificador de wildcards */
    enum Wildcards{
        WildcardNotUsed = 0,
        WildcardAny,
        WildcardAll,
		WildcardInvalid,
        WildcardCOUNT,
    };    
    
    /** Lista de operaciones pendientes
     *
     */
    enum PendingRequestType {
    	ReqPublish=0, 	//!< ReqPublish Solicitud de publicaci�n
		ReqSubscribe,   //!< ReqSubscribe Solicitud de suscripci�n
		ReqUnsubscribe, //!< ReqUnsubscribe Solicitud de unsuscripci�n
    };

    /** M�ximo n�mero de tokens permitidos en topic provider auto-gestionado */
    static const uint16_t DefaultMaxNumTokenEntries = (256 - WildcardCOUNT);    

    /** M�ximo n�mero de topics permitidos */
    static const uint16_t DefaultMaxNumTopics = 256;

    /** Lista de topics registrados */
    static List<MQ::Topic> * _topic_list;

    /** Puntero a la lista de topics proporcionados */
    static const char** _token_provider;
    static uint32_t _token_provider_count;
    static uint8_t _token_bits;
    static bool _tokenlist_internal;

	/** Mutex */
    static Mutex _mutex;

    /** L�mite de tama�o en nombres de topcis */
    static uint8_t _max_name_len;
 
    /** Flag de depuraci�n */
    static bool _defdbg;

    /** Estructura de datos de una solicitud pendiente
     *
     */
    struct PendingRequest_t{
    	PendingRequestType type;
    	char* topic;
    	void *msg;
    	uint32_t msg_len;
    	SubscribeCallback *sub_cb;
    	PublishCallback *pub_cb;
    };

    /** Lista de acciones pendientes por mutex bloqueado */
    static List<PendingRequest_t> *_pending_list;


    /** @fn findTopicByName 
     *  @brief Busca un topic por medio de su nombre, descendiendo por la jerarqu�a hasta
     *         llegar a un topic final.
     *  @param name nombre
     *  @return Pointer to the topic or NULL if not found
     */
    static MQ::Topic * findTopicByName(const char* name){
        MQ::Topic* topic = _topic_list->getFirstItem();
        while(topic){
            if(strcmp(name, topic->name)==0){
                return topic;
            }
            topic = _topic_list->getNextItem();
        }
        return NULL;
    }
 

    /** @fn generateTokens 
     *  @brief Genera los tokens no existentes en la lista auto-gestionada
     *  @param name nombre del topic a procesar
     *  @return True Topic insertado, False error en el topic
     */
    static bool generateTokens(const char* name){
        char* token = (char*)Heap::memAlloc(strlen(name));
        if(!token){
            return false;
        }
        uint8_t to=0, from=0;
        bool is_final = false;
        getNextDelimiter(name, &from, &to, &is_final);
        while(from < to){
            bool exists = false;
            for(int i = 0;i <_token_provider_count-WildcardCOUNT;i++){
                // si el token ya existe o es un wildcard, pasa al siguiente
                if(name[from] == '#' || name[from] == '+' || strncmp(_token_provider[i], &name[from], to-from)==0){
                	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "El token ya existe");
                    exists = true;
                    break;
                }
            }
            // tras recorrer todo el �rbol, si no existe lo a�ade
            if(!exists){
                char* new_token = (char*)Heap::memAlloc(1+to-from);
                if(!new_token){
                    Heap::memFree(token);
                    return false;
                }
                strncpy(new_token, &name[from], (to-from)); new_token[to-from] = 0;
                _token_provider[_token_provider_count-WildcardCOUNT] = new_token;
                _token_provider_count++;
                DEBUG_TRACE_D(_defdbg,"[MQLib].........", "A�adido token %s", new_token);
            }
            from = to+1;
            getNextDelimiter(name, &from, &to, &is_final);
        }
        Heap::memFree(token);
        return true;
    }

 

    /** @fn createTopicId 
     *  @brief Crea el identificador del topic. Los wildcards se sustituyen por el valor 0
     *  @param id Recibe el Identificador 
     *  @param name Nombre completo del topic
     */
    static void createTopicId(MQ::topic_t* id, const char* name){
        uint8_t from = 0, to = 0;
        bool is_final = false;
        DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Generando ID para el topic [%s]", name);
        int pos = 0; 
        // Inicializo el contenido del identificador para marcar como no usado
        for(int i=0;i<MQ::MAX_TOKEN_LEVEL;i++){
            id->tk[i] = WildcardNotUsed;
        }
        
        // obtiene los delimitadores para buscar tokens
        getNextDelimiter(name, &from, &to, &is_final);
        while(from < to){
        	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Procesando topic [%s], delimitadores (%d,%d)", name, from, to);
            uint32_t token = WildcardInvalid;
			// @05Mar2018.001 Verifico que sea un wildcard...
			// chequea si es un wildcard
			if(strncmp(&name[from], "+", to-from)==0){
				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Detectado wildcard (+) en delimitadores (%d,%d)", from, to);
				token = WildcardAny;
			}
			else if(strncmp(&name[from], "#", to-from)==0){
				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Detectado wildcard (#) en delimitadores (%d,%d)", from, to);
				token = WildcardAll;
			}
			else{
				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Analizando tokenX. Buscando token para delimitadores (%d,%d)", from, to);
				for(int i=0;i<(_token_provider_count - WildcardCOUNT);i++){
					// si encuentra el token... actualiza el id
					if(strncmp(_token_provider[i], &name[from], to-from)==0){
						DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Analizando tokenX. Encontrado token [%s]", _token_provider[i]);
						token  = i + WildcardCOUNT;
						break;
					}
				}
			}
			id->tk[pos] = (MQ::token_t)(token);

            // pasa al siguiente campo
            from = to+1;
            getNextDelimiter(name, &from, &to, &is_final);            
            pos ++;                        
        }
    }    
       
    
    /** @fn getNextDelimiter
     *  @brief Extrae delimitadores del nombre, token a token, desde la posici�n "from"
     *  @param name Nombre del topic a evaluar
     *  @param from Recibe el delimitador inicial y se usa como punto inicial de b�squeda
     *  @param to Recibe el delimitador final
     *  @param is_final Recibe el flag si es el subtopic final
     */
    static void getNextDelimiter(const char* name, uint8_t* from, uint8_t* to, bool* is_final){
        // se obtiene el tama�o total del nombre
        int len = strlen(name);
        // si el rango es incorrecto, no hace nada
        if(*from >= len){
            *from = len;
            *to = len;
            *is_final = true;
            return;
        }
        // se inicia la b�squeda, si parte de un espaciador, lo descarta
        if(name[*from] == '/'){
            (*from)++;
        }
        *to = *from;
        for(int i = *from; i <= len; i++){
            // si encuentra fin de trama, marca como final
            if(name[i] == 0){
                *to = i;
                break;
            }
            // si encuentra espaciador de tokens, marca como final
            else if(name[i] == '/'){
                *to = i;
                break;
            }
        }
        // si el punto final coincide con el tama�o de trama, marca como punto final
        if(*to >= len){
            *is_final = true;            
        }
    }      
       
    
    /** @fn matchIds
     *  @brief Compara dos identificadores. El de b�squeda con el encontrado. Si el encontrado
     *         contiene wildcards, habr� que tenerlos en cuenta.
     *  @param found_id Identificador encontrado
     *  @param search_id Identificador de b�squeda
     *  @return True si encajan, False si no encajan 
     */
    static bool matchIds(MQ::topic_t* found_id, MQ::topic_t* search_id){
        for(int i=0;i<MQ::MAX_TOKEN_LEVEL;i++){
        	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Comparando %d vs %d", found_id->tk[i], search_id->tk[i]);
			// si ha encontrado un wildcard All, es que coincide
			if(found_id->tk[i] == WildcardAll){
				return true;
			}

			// si ha llegado al final de la cadena de b�squeda sin errores, es que coincide...
			if(search_id->tk[i] == WildcardNotUsed){
				if(found_id->tk[i] != WildcardNotUsed)
					return false;
				return true;
			}

			// si no coinciden ni hay wildcards '+' involucrados, no hay coincidencia
			if(found_id->tk[i] != WildcardAny && found_id->tk[i] != search_id->tk[i]){
				return false;
			}

            // en otro caso, es que coinciden y por lo tanto sigue analizando siguientes elementos
        }        
        // si llega a este punto es que coinciden todos los niveles
        return true;
    }         


    /** A�ade una operaci�n a la lista de operaciones pendientes
     *
     *  @param type Tipo de operaci�n
     *  @param topic Nombre del topic
     *  @param data Datos del mensaje (s�lo para publicaciones)
     *  @param datasize Tama�o de los datos del mensaje (s�lo para publicaciones)
     *  @param pub_cb Callback de publicaci�n
     *  @param sub_cb Callback de suscripci�n
     *  @return C�digo de error
     */
    static int32_t addPendingRequest(PendingRequestType type, const char* topic, void* data, uint32_t datasize, PublishCallback *pub_cb, SubscribeCallback *sub_cb){
    	PendingRequest_t* req = (PendingRequest_t*)Heap::memAlloc(sizeof(PendingRequest_t));
    	MBED_ASSERT(req);
    	req->topic = (char*)Heap::memAlloc(strlen(topic)+1);
    	MBED_ASSERT(req->topic);
    	strcpy(req->topic, topic);
    	req->msg = data;
    	req->msg_len = datasize;
    	if(datasize){
    		req->msg = (void*)Heap::memAlloc(datasize);
    		MBED_ASSERT(req->msg);
    		memcpy(req->msg, data, datasize);
    		req->msg_len = datasize;
    	}
    	req->pub_cb = pub_cb;
    	req->sub_cb = sub_cb;
    	req->type = type;
    	DEBUG_TRACE_D(_defdbg,"[MQLib].........", "A�adiendo solicitud pendiente tipo %d en topic %s", (int)req->type, req->topic);
    	return _pending_list->addItem(req);
    }


    /** Procesa todas las operaciones pendientes, liberando los recursos asociados
     *
     */
    static void processPendingRequests(){
    	PendingRequest_t* req = _pending_list->getFirstItem();
    	while(req){
    		switch((int)req->type){
    			case ReqSubscribe:{
    				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Procesando solicitud pendiente tipo Subscribe (%d) en topic %s", (int)req->type, req->topic);
    				subscribeReq(req->topic, req->sub_cb, false);
    				break;
    			}
    			case ReqUnsubscribe:{
    				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Procesando solicitud pendiente tipo Unsubscribe (%d) en topic %s", (int)req->type, req->topic);
    				unsubscribeReq(req->topic, req->sub_cb, false);
    				break;
    			}
    			case ReqPublish:{
    				DEBUG_TRACE_D(_defdbg,"[MQLib].........", "Procesando solicitud pendiente tipo Publish (%d) en topic %s", (int)req->type, req->topic);
					publishReq(req->topic, req->msg, req->msg_len, req->pub_cb, false);
    				break;
    			}
    		}
    		// libera los recursos
    		if(req->msg && req->msg_len > 0){
    			Heap::memFree(req->msg);
    		}
    		Heap::memFree(req->topic);
    		_pending_list->removeItem(req);
    		Heap::memFree(req);
    		// Coge el siguiente elemento que volver� a ser el primero
    		req = _pending_list->getFirstItem();
    	}
    }
};




//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------




class MQClient{
public:
    
	/** @fn subscribe
     *  @brief Se suscribe a un tipo de topic, realizando una petici�n al broker
     *  @param name Nombre del topic
     *  @param subscriber Manejador de las actualizaciones del topic
     *  @return Resultado
     */
    static int32_t subscribe(const char* name, MQ::SubscribeCallback *subscriber){
		return MQBroker::subscribeReq(name, subscriber);
    }

	
    /** @fn unsubscribe
     *  @brief Finaliza la suscripci�n a un topic, realizando una petici�n al broker
     *  @param name Nombre del topic
     *  @param subscriber Suscriptor a eliminar de la lista de suscripci�n
     *  @return Resultado
     */
    static int32_t unsubscribe (const char* name, MQ::SubscribeCallback *subscriber){
		return MQBroker::unsubscribeReq(name, subscriber);
    }
	
		
    /** @fn publish 
     *  @brief Publica una actualizaci�n de un topic, realizando una petici�n al broker
     *  @param name Nombre del topic
     *  @param data Mensaje
     *  @param datasize Tama�o del mensaje
     *  @param publisher Callback de notificaci�n de la publicaci�n
     *  @param is_bridge Flag que indica si la publicaci�n proviene de un bridge
	 *	@return Resultado
     */
    static int32_t publish (const char* name, void *data, uint32_t datasize, MQ::PublishCallback *publisher){
        int32_t err = MQBroker::publishReq(name, data, datasize, publisher);
        executeBridge(name, data, datasize, publisher);
        return err;
    }  


    /** @fn republish
     *  @brief Publica un bridge
     *  @param name Nombre del topic
     *  @param data Mensaje
     *  @param datasize Tama�o del mensaje
     *  @param publisher Callback de notificaci�n de la publicaci�n
	 *	@return Resultado
     */
    static int32_t republish (const char* name, void *data, uint32_t datasize, MQ::PublishCallback *publisher){
        return MQBroker::publishReq(name, data, datasize, publisher);
    }

    
    /** @fn getTopicName 
     *  @brief Obtiene el nombre de un topic dado su id
     *  @param name Recibe el nombre asociado al id
     *  @param len Tama�o m�ximo aceptado para el nombre
     *  @param id Identificador del topic
     */
    static void getTopicName(char *name, uint8_t len, MQ::topic_t* id){
        MQBroker::getTopicNameReq(name, len, id);
    }        

    
    /** @fn getTopicId 
     *  @brief Obtiene el id de un topic dado su nombre
     *  @return Recibe el Identificador del topic
     *  @param name Nombre del topic
     */
    static void getTopicId(MQ::topic_t* id, const char* name){
        MQBroker::getTopicIdReq(id, name);
    }   

    
    /** @fn getMaxTopicLen 
     *  @brief Obtiene el n�mero m�ximo de caracteres que puede tener un topic
     *  @return N�mero de caracteres, incluyendo el '\0' final.
     */
    static uint8_t getMaxTopicLen(){
        return MQBroker::getMaxTopicLenReq();
    }        
    
    
    /** @fn isTokenRoot
     *  @brief Chequea si el topic comienza con un token dado
     *         "topic/a/b/c" con el token "/c" devolver� True, ya que el topic termina con ese token.
     *  @param topic Topic completo a comparar
     *  @param token token con el que comparar al principio del topic
     *  @return True si coincide, False si no coincide
     */
    static inline bool isTokenRoot(const char* topic, const char* token){
    	return ((strncmp(topic, token, strlen(token)) == 0)? true : false);
    }

    /** @fn isTopicToken 
     *  @brief Chequea si el token final de un topic coincide. Por ejemplo al comparar el topic
     *         "topic/a/b/c" con el token "/c" devolver� True, ya que el topic termina con ese token.
     *  @param topic Topic completo a comparar
     *  @param token token con el que comparar al final del topic
     *  @return True si coincide, False si no coincide
     */
    static inline bool isTopicToken(const char* topic, const char* token){
        return ((strcmp(topic + strlen(topic) - strlen(token), token) == 0)? true : false);
    }


    /** @fn getInternalTokenListReq
     *  @brief Obtiene la lista de tokens interna
     *  @param tklist Lista de tokens
     *  @param tkcount N�mero de tokens en la lista
     */
    static void getInternalTokenList(const char** &tklist, uint32_t &tkcount){
    	MQBroker::getInternalTokenListReq(tklist, tkcount);
    }


    /** @fn existsTopicReq
     *  @brief Chequea si un topic existe
     *  @param name Nombre del topic a chequear
     */
    static bool existsTopic(const char* name){
    	return MQBroker::existsTopicReq(name);
    }

    /**
     * A�ade un bridge a un topic dado
     * @param topic Topic origen
     * @param cb Callback para procesar el bridging
     */
    static int32_t addBridge(const char* topic, MQ::BridgeCallback* cb){
    	std::map<std::string, std::list<MQ::BridgeCallback*>*>::iterator it = _bridges.find(topic);
    	if(it!=_bridges.end()){
    		for(auto i = it->second->begin(); i != it->second->end(); ++i){
    			MQ::BridgeCallback* bc = (*i);
    			if(cb == bc){
    				return -2;
    			}
    		}
    		it->second->push_back(cb);
    		return 0;
    	}
    	// si no hay ning�n elemento en el mapa, con ese topic, lo crea
    	std::list<MQ::BridgeCallback*>* bclist = new std::list<MQ::BridgeCallback*>();
    	MBED_ASSERT(bclist);
    	bclist->push_back(cb);
    	_bridges.insert(std::pair<std::string, std::list<MQ::BridgeCallback*>*>(topic, bclist));
    	return 0;
    }

    /**
     * Elimina un bridge de un topic dado
     * @param topic Topic origen
     * @param cb Callback a eliminar
     */
    static int32_t removeBridge(const char* topic, MQ::BridgeCallback* cb){
    	std::map<std::string, std::list<MQ::BridgeCallback*>*>::iterator it = _bridges.find(topic);
    	if(it!=_bridges.end()){
    		for(auto i = it->second->begin(); i != it->second->end(); ++i){
    			MQ::BridgeCallback* bc = (*i);
    			if(cb == bc){
    				it->second->remove(bc);
    				// si se han eliminado todos los elementos, se elimina del mapa
    				if(it->second->size() == 0){
    					delete(it->second);
    					_bridges.erase(it);
    				}
    				return 0;
    			}
    		}
    	}
    	return -1;
    }

    /**
     * Ejecuta un bridge dado si existe
     *  @param name Nombre del topic
     *  @param data Mensaje
     *  @param datasize Tama�o del mensaje
     *  @param publisher Callback de notificaci�n de la publicaci�n
     */
    static void executeBridge(const char* name, void *data, uint32_t datasize, MQ::PublishCallback *publisher){
    	std::string topic(name);
        std::string delimiter = "/";
        std::vector<std::string> topicSplit;

        int pos = 0;
        std::string token;
        while ((pos = topic.find(delimiter)) != std::string::npos) {
            token = topic.substr(0, pos);
            topicSplit.push_back(token);
            topic.erase(0, pos + delimiter.length());
            if((pos = topic.find(delimiter)) == std::string::npos)
                topicSplit.push_back(topic);
        }

        std::map<std::string, std::list<MQ::BridgeCallback*>*>::iterator it;
        for ( it = _bridges.begin(); it != _bridges.end(); it++ )
        {
            std::string topicB(it->first);
            std::string token;
            int topicPos = 0;
            bool proccess = true;
            bool defProccess = false;
            while ((pos = topicB.find(delimiter)) != std::string::npos) {
                token = topicB.substr(0, pos);
                if(token.compare("#")==0){
                    defProccess = true;
                    break;
                }
                else if(topicPos >= topicSplit.size() || (token.compare(topicSplit[topicPos])!=0 && token.compare("+")!=0)){
                    proccess = false;
                    break;
                }
                topicPos++;
                topicB.erase(0, pos + delimiter.length());
            }

            if(defProccess || (proccess && topicPos+1 == topicSplit.size() && (topicB.compare(topicSplit[topicPos])==0 || topicB.compare("+")==0))){
                for(auto i = it->second->begin(); i != it->second->end(); ++i){
                    MQ::BridgeCallback* bc = (*i);
                    bc->call(name, data, datasize, publisher);
                }
            }
        }
    }


private:
    static std::map<std::string, std::list<MQ::BridgeCallback*>*> _bridges;

};





//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------




class MQBridge{
public:

	/** Constructor
	 *
	 */
	MQBridge(bool defdbg = false) : _defdbg(defdbg){
		_brsubCb = callback(this, &MQBridge::bridgeSubscriptionCb);
		_brpubCb = callback(this, &MQBridge::bridgePublicationCb);
		_bridge_list = new List<Bridge_t>();
		MBED_ASSERT(_bridge_list);
	}


	/** Destructor
	 *
	 */
	virtual ~MQBridge(){}


	/** Crea un nuevo bridge
	 *
	 * @param from Topic origen
	 * @param to Topic al que redireccionar
	 * @return Resultado
	 */
	int32_t addBridge(const char* from, const char* to, Callback<void()> cb){
		DEBUG_TRACE_D(_defdbg,"[MQBridge]......", "Creando brige %s -> %s", from, to);
		_mtx.lock();
		Bridge_t* br = (Bridge_t*)Heap::memAlloc(sizeof(Bridge_t));
		MBED_ASSERT(br);
		br->topicFrom = from;
		br->topicTo = to;
		_bridge_list->addItem(br);
		int32_t rc = MQClient::subscribe(br->topicFrom, &_brsubCb);
		_mtx.unlock();
		return rc;
	}


	/** Elimina un bridge
	 *
	 * @param from Topic origen
	 * @return Resultado
	 */
	int32_t removeBridge(const char* from){
		int32_t rc = NOT_FOUND;
		DEBUG_TRACE_D(_defdbg,"[MQBridge]......", "Eliminando brige %s", from);
		_mtx.lock();
		Bridge_t* br = _bridge_list->getFirstItem();
		while(br){
			if(strcmp(from, br->topicFrom) == 0){
				if((rc = MQClient::unsubscribe(br->topicFrom, &_brsubCb)) == SUCCESS){
					DEBUG_TRACE_D(_defdbg,"[MQBridge]......", "Bridge eliminado %s -> %s", from, br->topicTo);
					_bridge_list->removeItem(br);
					Heap::memFree(br);
					break;
				}
			}
			br = _bridge_list->getNextItem();
		}
		_mtx.unlock();
		return rc;
	}

private:
    struct Bridge_t{
    	const char* topicFrom;
    	const char* topicTo;
    };

    bool _defdbg;
    List<Bridge_t>* _bridge_list;
    SubscribeCallback _brsubCb;
    PublishCallback _brpubCb;
    Mutex _mtx;


    /** Suscripci�n a topics y reenv�o
     *
     * @param topic Topic suscrito
     * @param msg Mensaje
     * @param msg_len Tama�o del mensaje
     */
    virtual void bridgeSubscriptionCb(const char* topic, void* msg, uint16_t msg_len){
		_mtx.lock();
		Bridge_t* br = _bridge_list->getFirstItem();
		while(br){
			if(strcmp(topic, br->topicFrom) == 0){
				break;
			}
			br = _bridge_list->getNextItem();
		}
		_mtx.unlock();
		if(br){
			DEBUG_TRACE_D(_defdbg,"[MQBridge]......", "Redireccionando %s -> %s", br->topicFrom, br->topicTo);
			MQClient::publish(br->topicTo, msg, msg_len, &_brpubCb);
		}
    }


    /** Callback tras publicaci�n del bridging
     *
     * @param name Topic name
     * @param result Resultado
     */
    virtual void bridgePublicationCb(const char* name, int32_t result){
    }
};


} /* End of namespace MQ */

#endif /* MSGBROKER_H_ */
