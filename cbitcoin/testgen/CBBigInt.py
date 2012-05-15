import random,struct
li = []
li2 = []
li3 = []
li4 = []
li5 = []
li6 = []
li7 = []
li8 = []
for x in xrange(100):
	a = 0
	lii = []
	for y in xrange(32):
		lii.append(random.randint(0 + (1 if (y == 0) else 0),255))
		a += lii[-1] << (31-y)*8
	li.append(lii)
	li2.append(random.randint(1,255))
	li3.append(a/li2[-1])
	li5.append(a*li2[-1])
	li4.append(a % li2[-1])
	li7.append(a)
	li8.append(a - li2[-1]);
for x in xrange(100):
	li6.append(li7[x]+li7[99-x])
print "u_int8_t bytes[][32] = {",
for x in li:
	print "{",
	for y in xrange(31,-1,-1): #Little endian
		print hex(x[y]),
		print ",",
	print "},",
print "};"
print "u_int8_t nums[] = {",
for x in li2:
	print hex(x),
	print ",",
print "};"
print "u_int8_t mod_results[] = {",
for x in li4:
	print str(hex(x))[:-1],
	print ",",
print "};"
print "u_int8_t divide_results[][32] = {",
for x in li3:
	print "{",
	for y in xrange(31,-1,-1):
		print str(hex((x & (0xFF << 8*(31-y))) >> 8*(31-y)))[:-1],
		print ",",
	print "},",
print "};"
print "u_int8_t multiply_results[][33] = {",
for x in li5:
	print "{",
	for y in xrange(32,-1,-1):
		print str(hex((x & (0xFF << 8*(32-y))) >> 8*(32-y)))[:-1],
		print ",",
	print "},",
print "};"
print "u_int8_t add_results[][33] = {",
for x in li6:
	print "{",
	for y in xrange(32,-1,-1):
		print str(hex((x & (0xFF << 8*(32-y))) >> 8*(32-y)))[:-1],
		print ",",
	print "},",
print "};"
print "u_int8_t sub_results[][33] = {",
for x in li8:
	print "{",
	for y in xrange(32,-1,-1):
		print str(hex((x & (0xFF << 8*(32-y))) >> 8*(32-y)))[:-1],
		print ",",
	print "},",
print "};"
print "divide_results verify"
for x in li3:
	print hex(x)
print "end divide_results verify"
print "multiply_results verify"
for x in li5:
	print hex(x)
print "end multiply_results verify"