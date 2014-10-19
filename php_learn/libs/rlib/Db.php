<?php

/**
 * @Deprecated currently
 */

/**
 * A simplified wrapper for php mysqli API.
 * @author roxma
 */

class DbConfig{
	var $host = '127.0.0.1';
	var $port = 3306;
	var $userName = "root";
	var $password = "";
	var $useDatabase = "";
	var $charset = "utf-8";
};

class DbEscapeString{
	public $value;
	function __construct($value){
		$this->value=$value;
	}
};

/**
 * @param string to be escaped to prevent sql injection.
 * @return DbEscapeString
 */
function DbEscape($str){
	return new DbEscapeString($str);
}
/**
 * The same as DbEscape
 * @param unknown $str
 * @return DbEscapeString
 */
function DbWrapStr($str){
	return new DbEscapeString($str);
}

class DbResult{
	public $errorCode = 0;
	public $errorMessage = "";
	public $statement = "";
	
	public $affectedRows = 0;

	
	/**
	 * for example:
	 * array(
	 *     array("id"=>"1", "value"=>"value1")
	 *   , array("id"=>"2", "value"=>"value2")
	 * );
	 */
	public $selectResult = array();
};


class Db{
	
	private $dbConfig;
	
	function __construct(DbConfig $dbConfig){
		$this->dbConfig = $dbConfig;
	}
	
	function __destruct(){
		$this->Reset();
	}
	
	private $mysqlLink = NULL;
	
	/**
	 * @return DbResult
	 */
	private function Init(){
		
		$result = new DbResult();
		
		if($this->mysqlLink == NULL){
			$this->mysqlLink = new mysqli(
							  $this->dbConfig->host
							, $this->dbConfig->userName
							, $this->dbConfig->password
							, $this->dbConfig->useDatabase
							, $this->dbConfig->port
						);
			if($this->mysqlLink->connect_errno){
				
				$result->errorCode = __LINE__;
				$result->errorMessage = "[" . __FILE__ . "]:[" . __FUNCTION__ . "]: "
							. 'Could not connect: ' . $this->mysqlLink->connect_error;
				
				$this->mysqlLink->close();
				$this->mysqlLink = NULL;
				
				return $result;
			}
			
			$setCharset = $this->mysqlLink->set_charset($this->dbConfig->charset);
			if(!$setCharset){
				$result->errorCode = __LINE__;
				$result->errorMessage = "[" . __FILE__ . "]:[" . __FUNCTION__ . "]: "
							. 'Could not set charset to ' .  $this->dbConfig->charset 
							. ", error: " . $this->mysqlLink->error;
				
				$this->mysqlLink->close();
				$this->mysqlLink = NULL;
				
				return $result;
			}
			
			return $result;
			
		}else{ // $this->mysqlLink != NULL
			return $result;
		}
	}
	
	/**
	 * Reset db connection.
	 */
	public function Reset(){
		if($this->mysqlLink){
			$this->mysqlLink->close();
			$this->mysqlLink = NULL;
		}
	}
	
	public function GetLastError(){
		return $this->mysqlLink->error;
	}

	private function QueryStatementAppend(& $query, $append){
		if($append instanceof DbEscapeString){
			$query = $query . '"' . $this->mysqlLink->real_escape_string( $append->value ) . '"';
		}else if(is_array($append)){
			foreach ($append as $item){
				$this->QueryStatementAppend($query, $item);
			}
		}else{
			$query = $query . $append;
		}
	}
	
	/**
	 * @return DbResult
	 */
	public function Execute(){
	
		$result = new DbResult();
		
		$initResult = $this->Init();
		if($initResult->errorCode){
			
			$result->errorCode = __LINE__;
			$result->errorMessage = "[" . __FILE__ . "]:[" . __FUNCTION__ . "]: "
					. 'init failed: ' . $initResult->errorCode . ":" . $initResult->errorMessage;
			
			return $result;
		}
		
		$query = "";
		
		for($i = 0; $i<func_num_args(); ++$i){
			$param = func_get_arg($i);
			
			if(!isset($param)){
				$result->errorCode = __LINE__;
				$result->errorMessage = "[" . __FILE__ . "]:[" . __FUNCTION__ . "]: "
					. "invalid parameter index " . $i;
				return $result;
			}
			
			$this->QueryStatementAppend($query, $param);
		}
		
		$result->statement = $query;
		
		$queryResult = $this->mysqlLink->query($query);
		
		if($queryResult==FALSE){
			$result->errorCode = __LINE__;
			$result->errorMessage = "[" . __FILE__ . "]:[" . __FUNCTION__ . "]: "
					. "query error[" . $this->mysqlLink->errno . "] " . $this->mysqlLink->error;
			return $result;
		}
		

		$result->affectedRows = $this->mysqlLink->affected_rows;
	
		
		if($queryResult instanceof mysqli_result){

			$num_rows = $queryResult->num_rows;
	
			while($row = $queryResult->fetch_array(MYSQLI_ASSOC)){
				$result->selectResult[] = $row;
			}
			
			$queryResult->free();
			
			return $result;
		}
		
		return $result;
	}

	/**
	 * begin transaction
	 * @return DbResult
	 */
	public function Begin(){
		return $this->Execute("start transaction");
	}
	
	/**
	 * @return DbResult
	 */
	public function RollBack(){
		return $this->Execute("rollback");
	}
	
	/**
	 * @return DbResult
	 */
	public function Commit(){
		return $this->Execute("commit");
	}
	
};

?>
