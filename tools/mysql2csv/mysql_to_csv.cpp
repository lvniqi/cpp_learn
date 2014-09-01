#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>

class CsvWriter{

	std::ostream* pOstream;

	// from http://stackoverflow.com/questions/5343190
	std::string ReplaceString(
			  std::string subject
			, const std::string& search
			, const std::string& replace) {
    		size_t pos = 0;
    		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
    		return subject;
	}

	std::string MakeCell(const std::string& i_value){
		return "\"" + ReplaceString(i_value,"\"","\"\"" ) + "\"";
	}

public:

	/**
 	 * @param or_stream [out ref] 
 	 * @note The stream should be opened in binary mode.
 	 */
	CsvWriter(std::ostream* or_stream):pOstream(or_stream){}

	/**
	 * @return error code
	 */
	int WriteRow(const std::vector<std::string>& i_cells){
		for(size_t i=0; i<i_cells.size(); ++i){
			if(i!=0) this->pOstream->write(",",1);
			std::string cell = MakeCell(i_cells[i]);
			this->pOstream->write(cell.c_str(),cell.length());
		}
		this->pOstream->write("\r\n",2); // CR LF
		if(pOstream->fail())
			return __LINE__;
		else
			return 0;
	}
};

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <mysql/mysql.h>

void ShowUsage(const std::map<std::string,std::string>& i_args){
	std::cerr<<"usage: this_bin ";
	for(std::map<std::string,std::string>::const_iterator it=i_args.begin(); it!=i_args.end(); ++it){
		std::cerr<< it->first <<"=["<< it->first<<"] ";
	}
	std::cerr<<std::endl;
}

int ProcessArgs(int i_argc, char ** i_argv, std::map<std::string,std::string>& io_args, std::string& o_errorMessage){
	if(i_argc<=1){
		o_errorMessage = "no arguments";
		ShowUsage(io_args);
		return __LINE__;
	}
	
	for(int i=1; i<i_argc; ++i){
		std::string strPair = i_argv[i];
		
		// get key and value
		std::string::size_type pos = strPair.find_first_of("=");
		if(pos == std::string::npos){
			o_errorMessage = "Invalid parameter string \""+strPair+"\", it shoud be in this format \"key=value\"";
			return __LINE__;
		}
		std::string key = strPair.substr(0, pos);
		std::string value = strPair.substr(pos+1,strPair.length()-1-key.length());
		std::clog<<key<<"="<<value<<std::endl;
		
		if(io_args.count(key)<=0){
			o_errorMessage = "The parameter [" + key + "] is not supported";
			return __LINE__;
		}
		
		io_args[key] = value;
	}
	
	return 0;
}

int main(int argc,char**argv){

	std::map<std::string,std::string> args;
	args["host"] = "127.0.0.1";
	args["port"] = "3306";
	args["user"] = "root";
	args["passwd"] = "";
	args["charset"] = "utf8";

	args["db"] = "";
	args["execute"] = "";
	
	args["null_cell_value"] = "NULL";
	
	args["output"] = "";
	
	std::string errorMessage;
	int errorCode = ProcessArgs(argc,argv,args,errorMessage);
	if(errorCode){
		std::cerr<<"ERROR "<<errorCode<<": "<<errorMessage<<std::endl;
		return errorCode;
	}

	
	// open output stream
	std::ostream* pStream = NULL;
	std::ofstream file;
	if(args["output"] != ""){
		file.open(args["output"].c_str(),std::ofstream::binary);
		pStream = &file;
	}else{
		pStream = &std::cout;
	}

	CsvWriter writer(pStream);

	
	MYSQL mysql;
	mysql_init(&mysql);
	int port = 0;
	std::istringstream issPort(args["port"]);
	issPort>>port;
	if(issPort.fail()){
		std::cerr<<"ERROR "<<": "<<"invalid parameter port, should be an integer."<<std::endl;
		return __LINE__;
	}
	const char * pDbCstr = "";
	if(args["db"] != ""){
		pDbCstr = args["db"].c_str();
	}

	// connection
	int charsetErr = mysql_options(&mysql,MYSQL_SET_CHARSET_NAME,args["charset"].c_str());
	if(charsetErr){
		std::cerr<<"ERROR : cannot set connection charset"<<std::endl;
		mysql_close(&mysql);
		return __LINE__;
	}
	MYSQL* connectSuccess = mysql_real_connect(&mysql,args["host"].c_str(),args["user"].c_str(),args["passwd"].c_str(),pDbCstr,port,NULL,0);
	if(connectSuccess==NULL){
		std::cerr<<"ERROR : cannot connect to mysql."<<std::endl;
		return __LINE__;
	}

	std::string sql = args["execute"];
	if( sql == ""){
		std::cerr<<"ERROR "<<": "<<"The statement to be execute cannot be empty, please set the execute parameter"<<std::endl;
		mysql_close(&mysql);
		return __LINE__;
	}

	int queryErr = mysql_query(&mysql, sql.c_str());
	if(queryErr){
		std::cerr<<"ERROR "<<queryErr<<": query failed."<<std::endl;
		return __LINE__;
	}

	std::vector<std::string> header;

	MYSQL_RES *result = mysql_store_result(&mysql);
	int fieldcount = mysql_num_fields(result);
	for(int i=0; i<fieldcount; ++i){
		MYSQL_FIELD *field = mysql_fetch_field_direct(result,i);
		header.push_back(std::string( field->name ));
	}

	writer.WriteRow(header);

	for(MYSQL_ROW row = mysql_fetch_row(result); row!=NULL; row = mysql_fetch_row(result)){
		std::vector<std::string> csvRow;
		unsigned long *lengths = mysql_fetch_lengths(result);
		for(int i=0; i<fieldcount; ++i){
			if(row[i]){
				csvRow.push_back(std::string(row[i]));
			}else{
				// null value
				csvRow.push_back(args["null_cell_value"]);
			}
		}
		writer.WriteRow(csvRow);
	}
	
	file.close();
	return 0;
}


