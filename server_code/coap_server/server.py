import datetime
import logging
import asyncio

from mysql.connector import pooling
import aiocoap.resource as resource
from aiocoap.numbers.contentformat import ContentFormat
import aiocoap
import time


# logging setup
logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server").setLevel(logging.DEBUG)

ip_to_number = {}
next_number = 1  # Start counting from 1

def get_or_assign_number(ip_address):
    global next_number

    if ip_address not in ip_to_number:
        ip_to_number[ip_address] = next_number
        next_number += 1

    return ip_to_number[ip_address]

attemps = 0
while attemps < 10:
    try:
        pool = pooling.MySQLConnectionPool(
            pool_name="mypool",
            pool_size=5,
            pool_reset_session=True,
            host="172.20.0.11",
            user="root",
            password="manexmanex",
            database="iotDB",
            auth_plugin='mysql_native_password'
        )
        break
        print("Connection established correctly")
    except:
        attemps += 1
        print("Error: Couldn't connect to database")
        time.sleep(5)

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
        insert_data(payload, self.data_type, get_or_assign_number(request.remote))
        return aiocoap.Message(code=aiocoap.numbers.codes.CREATED, payload=f"ALL OK {payload}".encode('utf-8'))


def insert_data(payload, data_type,ip_id):
    try:
        with pool.get_connection() as connection:
            cursor = connection.cursor()
            query = f"INSERT INTO {data_type}_data ({data_type}, idSensor, time) VALUES ({payload}, {ip_id}, CURRENT_TIMESTAMP);"
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
