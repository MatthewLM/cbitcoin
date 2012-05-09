import random,struct
li = []
li2 = []
li3 = []
li4 = []
for x in xrange(100):
	a = 0
	lii = []
	for y in xrange(32):
		lii.append(random.randint(0 + (1 if (y == 0) else 0),255))
		a += lii[-1] << (31-y)*8
	li.append(lii)
	li2.append(random.randint(1,255))
	li3.append(a/li2[-1])
	li4.append(a % li2[-1])
print "u_int8_t bytes[][32] = {",
for x in li:
	print "{",
	for y in xrange(32):
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
	for y in xrange(32):
		print str(hex((x & (0xFF << 8*(31-y))) >> 8*(31-y)))[:-1],
		print ",",
	print "},",
print "};"
print "divide_results verify"
for x in li3:
	print hex(x)
print "end divide_results verify"