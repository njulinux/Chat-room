all : server_app client_app 
TARGET1 = server_app
TARGET2 = client_app
SRC1 = server.c 
SRC2 = client.c 


$(TARGET1) : $(SRC1)
	gcc -o $(TARGET1) $(SRC1) -lpthread

$(TARGET2) : $(SRC2)
	gcc -o $(TARGET2) $(SRC2) -lpthread

clean :
	rm -f $(TARGET1) $(TARGET2)
	
