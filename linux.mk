#--- Environment check ---
#-------------------------

#--- Server environment ---
#--------------------------

 C_BASE=
 L_BASE=

TMP=tmp
OUT=out
TEST=test
WORK=/opt/ethereum_mirror

WARNINGS= -Wformat \
          -Wunused \
          -Wuninitialized \
          -Wshadow \
          -Wno-write-strings \
          -Wno-format-truncation
OPTIMISE= -O0

DEFINES=-DUNIX -DLINUX $(OPTIMISE) $(WARNINGS) $(C_BASE)

#--- ABTP environment ---

ABTP_H=-I$(COMMON_LIB_PATH)/grabtp
ABTP_L=$(COMMON_LIB_PATH)/grabtp/grabtp.a

#--- SQL environment ---

SQL_H=-I$(COMMON_LIB_PATH)/grsql -I/usr/include/postgresql
SQL_L=$(COMMON_LIB_PATH)/grsql/grsql.a

#--- DCL environment ---

DCL_H=-I$(COMMON_LIB_PATH)/dcl
DCL_L=$(COMMON_LIB_PATH)/dcl/dcl.a

#--- Project components ---
#--------------------------

default : init install

init : $(TMP) $(OUT)

$(TMP) :
	mkdir $(TMP)

$(OUT) :
	mkdir $(OUT)

all      : clean $(OUT)/ethereum_mirror

$(OUT)/ethereum_mirror : $(TMP)/ethereum_mirror.o $(TMP)/ethereum_unix.o $(TMP)/emir_common.o $(TMP)/emir_crypt.o $(TMP)/emir_files.o $(TMP)/emir_sysconfig.o $(TMP)/emir_members.o $(TMP)/emir_deals.o $(TMP)/emir_scan.o $(TMP)/emir_sentry.o
	g++ -o $(OUT)/ethereum_mirror $(TMP)/ethereum_unix.o $(TMP)/ethereum_mirror.o $(TMP)/emir_common.o $(TMP)/emir_crypt.o $(TMP)/emir_files.o \
		$(TMP)/emir_sysconfig.o $(TMP)/emir_members.o $(TMP)/emir_deals.o $(TMP)/emir_scan.o $(TMP)/emir_sentry.o \
		$(ABTP_L) $(SQL_L) -lpq

$(TMP)/ethereum_unix.o : Ethereum_Mirror_unix.cpp
	g++ -c -o $(TMP)/ethereum_unix.o $(DEFINES) $(SQL_H) $(ABTP_H) Ethereum_Mirror.cpp

$(TMP)/ethereum_mirror.o : Ethereum_Mirror.cpp
	g++ -c -o $(TMP)/ethereum_mirror.o $(DEFINES) $(SQL_H) $(ABTP_H) Ethereum_Mirror_unix.cpp

$(TMP)/emir_common.o : EMIR_common.cpp
	g++ -c -o $(TMP)/emir_common.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_common.cpp

$(TMP)/emir_crypt.o : EMIR_crypt.cpp
	g++ -c -o $(TMP)/emir_crypt.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_crypt.cpp

$(TMP)/emir_files.o : EMIR_files.cpp
	g++ -c -o $(TMP)/emir_files.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_files.cpp

$(TMP)/emir_sysconfig.o : EMIR_sysconfig.cpp
	g++ -c -o $(TMP)/emir_sysconfig.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_sysconfig.cpp

$(TMP)/emir_members.o : EMIR_members.cpp
	g++ -c -o $(TMP)/emir_members.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_members.cpp

$(TMP)/emir_deals.o : EMIR_deals.cpp
	g++ -c -o $(TMP)/emir_deals.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_deals.cpp

$(TMP)/emir_scan.o : EMIR_scan.cpp
	g++ -c -o $(TMP)/emir_scan.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_scan.cpp

$(TMP)/emir_sentry.o : EMIR_sentry.cpp
	g++ -c -o $(TMP)/emir_sentry.o $(DEFINES) $(SQL_H) $(ABTP_H) EMIR_sentry.cpp

#$(TMP)/emir_oracle.o : EMIR_oracle.cpp
#	g++ -c -o $(TMP)/emir_oracle.o $(DEFINES) $(DCL_H) EMIR_oracle.cpp

#--- Cleaning ---
#----------------

clean:
	rm -f $(TMP)/*.* $(OUT)/*.*

#--- Copy to installing folder ---
#---------------------------------

install: init all
	cp $(OUT)/ethereum_mirror $(TEST)
