import httplib
import socket
import json
from printer import *

__IP_ADDR = "192.168.1.232"
__PORT = "16021"

def send(verb, endpoint, body):
    __API_LISTENER = __IP_ADDR + ":" + __PORT
    iprint("sending to: " + __API_LISTENER)
    try:
        conn = httplib.HTTPConnection(__API_LISTENER)
        if len(body) != 0:
            conn.request(
                verb,
                endpoint,
                body,
                {"Content-Type": "application/json"}
            )
        else :
            conn.request(verb, endpoint)

        response = conn.getresponse()
        body = response.read()
        return response.status, response.reason, body
    except (httplib.HTTPException, socket.error) as ex:
        print ("Error: %s" % ex)
        quit()

def setIPAddr(ip_addr):
    global __IP_ADDR
    __IP_ADDR = ip_addr

def v1getUri(auth_token, endpoint):
    uri = "/api/v1/" + auth_token
    if len(endpoint) == 0 or not endpoint[0] == '/':
        uri += "/"
    uri += endpoint
    return uri

def request_token():
    parsed_json = ""
    try:
        status, reason, body = send("POST", "/api/v1/new", "")
        parsed_json = json.loads(body)
    except:
        pass

    if 'auth_token' not in parsed_json:
        return False, ""

    auth_token = parsed_json['auth_token']
    if status == 200 and reason == "OK" and len(auth_token) == 32:
        return True, auth_token
    else:
        return False, ""