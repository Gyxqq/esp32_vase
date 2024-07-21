# 用于生成光敏电阻阻值与光照强度表的脚本，用于光强插值计算
list0=[]
for i in range(1, 1000):
    y=1000*(4*10**0.4)*i**(-0.6021)
    list0.append(int (y))
print(list0)
last=list0[0]
list2=[]
list2.append((1, last))
for i in range(len(list0)):
    if list0[i]==last:
        continue
    else:
        list2.append((i+1, list0[i]))
        last=list0[i]
print(list2)
f0=open('light_sensor_list.c', 'w')
f0.write('struct light_sensor_list{\n')
f0.write('    int lux;\n')
f0.write('    int r;\n')
f0.write('};\n')
f0.write('struct light_sensor_list light_sensor_list[]={\n')
for i in range(len(list2)):
    f0.write('    {%d, %d},\n' % (list2[i][0], list2[i][1]))
f0.write('};\n')

f0.close()
