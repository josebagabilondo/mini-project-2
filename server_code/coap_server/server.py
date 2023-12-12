import datetime
import logging
import asyncio

import mysql.connector
import aiocoap.resource as resource
from aiocoap.numbers.contentformat import ContentFormat
import aiocoap

# logging setup
logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)



def create_connection():
    try:
        connection = mysql.connector.connect(
            host="172.20.0.11",
            user="manex",
            password="manexmanex",
            database="iotDB",
            auth_plugin='mysql_native_password'
        )

        return connection
    except mysql.connector.Error as err:
        print(f'Error: {err}')
        return -1
    except:
        print("Could not connect to mysql")
        
connection = create_connection()

class test_handler(resource.Resource):
    async def render_get(self, request):
        return aiocoap.Message(payload="Terve, everything good!".encode('utf8'))

class handler(resource.Resource):
    def __init__(self,data_type):
        super().__init__()
        self.data_type = data_type
        
    async def render_post(self, request):
        payload = request.payload.decode('utf-8')
        print(f"POST request received with payload: {payload}")
        insert_data(payload, self.data_type)
        return aiocoap.Message(code=aiocoap.numbers.codes.CREATED, payload=f"ALL OK {payload}".encode('utf-8'))


def insert_data(payload, data_type):
    try:
        connection = mysql.connector.connect(
            host="172.20.0.11",
            user="manex",
            password="manexmanex",
            database="iotDB",
            auth_plugin='mysql_native_password'
        )
        cursor = connection.cursor()
        query = f"INSERT INTO {data_type}_data ({data_type}, ts) VALUES ({payload}, CURRENT_TIMESTAMP);"
        cursor.execute(query)
        connection.commit()
    except mysql.connector.Error as err:
        print(f'Error: {err}')
    except:
        print(f"Query error: INSERT INTO {data_type}_data ({data_type}, ts) VALUES ({payload}, CURRENT_TIMESTAMP);")
        connection = create_connection()


async def main():
    # Resource tree creation
    root = resource.Site()
    root.add_resource(["test"], test_handler())
    root.add_resource(["temperature"], handler("temperature"))
    root.add_resource(["light"], handler("light"))
    root.add_resource(["pressure"], handler("pressure"))


    await aiocoap.Context.create_server_context(root)

    # Run forever
    await asyncio.get_running_loop().create_future()

if __name__ == "__main__":
    asyncio.run(main())
