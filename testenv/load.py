import pymonetdb
table_name = "regextest"

def load(file_path):
	connection = pymonetdb.connect(username="monetdb", password="monetdb", hostname="localhost", database="demo")
	cursor = connection.cursor()
	try:
		cursor.execute("create table if not exists %s (name VARCHAR(256));" % table_name)
		connection.commit()
	except:
		print "table exists"
	f = open(file_path)
	for line in f:
		pos = line.find('|')
		line = line[pos+1:]
		line = line.strip()
		cmd = "INSERT INTO %s VALUES ('%s');" % (table_name, line)
		cursor.execute(cmd)
	connection.commit()
	cmd = "select count(*) from %s" % table_name
	cursor.execute(cmd)
	print cursor.fetchone()
	
file_path =  "/home/xzhang3/testenv/string-generator/generate.out"
load(file_path)


