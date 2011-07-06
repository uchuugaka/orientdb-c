#include "o_query_engine_remote.h"
#include "o_query_engine_internal.h"
#include "o_query_internal.h"
#include "o_storage_remote_internal.h"
#include "o_remote_protocol_specs.h"
#include "o_connection_remote.h"
#include "o_memory.h"
#include "o_output_stream_byte.h"
#include "o_raw_buffer_byte.h"
#include "o_exceptions.h"
#include "o_exception.h"
#include <string.h>

struct o_query_engine_remote
{
	struct o_query_engine engine;
	struct o_storage_remote * storage;
};

void o_query_engine_remote_record_result(struct o_connection_remote * connection, void * add_info, query_result_callback callback)
{
	//CLASS!! RIQUERED FOR THE PROTOCOLL, I IGNORE
	o_connection_remote_read_short(connection);
	int size;
	char type = o_connection_remote_read_byte(connection);
	short rec_cl = o_connection_remote_read_short(connection);
	long long rec_pos = o_connection_remote_read_long64(connection);
	int version = o_connection_remote_read_int(connection);
	unsigned char * o_p = o_connection_remote_read_bytes(connection, &size);
	struct o_raw_buffer * buff = o_raw_buffer_byte(type, version, o_p, size);
	callback(add_info, o_record_id_new(rec_cl, rec_pos), buff);
}
void o_query_engine_remote_query_parameter(struct o_query_engine * engine, struct o_query * query, struct o_document_value ** parameters, void * add_info,
		query_result_callback callback)
{
	struct o_query_engine_remote *engine_remote = (struct o_query_engine_remote *) engine;
	struct o_connection_remote * conn = o_storage_remote_begin_write(engine_remote->storage, COMMAND);
	o_connection_remote_write_byte(conn, 's');
	struct o_output_stream *str = o_output_stream_byte_buffer();
	o_query_seriealize(query, str);
	int cont_size = 0;
	unsigned char *content = o_output_stream_byte_content(str, &cont_size);
	o_connection_remote_write_bytes(conn, content, cont_size);
	o_storage_remote_end_write(engine_remote->storage, conn);
	o_output_stream_free(str);


	conn =o_storage_remote_begin_response(engine_remote->storage);
	char response = o_connection_remote_read_byte(conn);
	try
	{
		switch (response)
		{
		case 'n':
		{
			int size;
			unsigned char * readed = o_connection_remote_read_bytes(conn, &size);
			o_free(readed);
		}
			break;
		case 'r':
		{
			o_query_engine_remote_record_result(conn, add_info, callback);
		}
			break;
		case 'l':
		{
			int size = o_connection_remote_read_int(conn);
			while (size-- > 0)
			{
				o_query_engine_remote_record_result(conn, add_info, callback);
			}

		}
			break;
		}
		o_storage_remote_end_read(engine_remote->storage, conn);
	}
	catch( struct o_exception ,ex)
	{
		o_storage_remote_end_read(engine_remote->storage, conn);
		throw(ex);
	}
	end_try;


}

void o_query_engine_remote_free(struct o_query_engine * engine)
{
	o_free(engine);
}

struct o_query_engine * o_query_engine_remote_new(struct o_storage_remote * remote)
{
	struct o_query_engine_remote * remeng = o_malloc(sizeof(struct o_query_engine_remote));
	memset(remeng, 0, sizeof(struct o_query_engine_remote));
	remeng->storage = remote;
	remeng->engine.o_query_engine_query_parameter = o_query_engine_remote_query_parameter;
	remeng->engine.o_query_engine_free = o_query_engine_remote_free;
	return &remeng->engine;
}

