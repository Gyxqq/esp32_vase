import json

str = """{
"msg": "SUCCESS",
"plant_id": "04684a1-4fc7c99",
"temperature": 24.18,
"humidity": 99.82,
"luminance": 50
}"""
j=json.loads(str)
print(j['msg'])
print(j['plant_id'])
print(j['temperature'])