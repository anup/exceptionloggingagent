DROP DATABASE exception;
CREATE DATABASE exception;
USE exception;
CREATE TABLE tbl_ExceptionLog
(
	ExceptionId INT NOT NULL AUTO_INCREMENT,
	TimeStamp DATETIME, 
	ExceptionMsg VARCHAR(500),
	ExceptionType VARCHAR(500),
	SourceFileName VARCHAR(500),
	ThreadName VARCHAR(500),
	CatchMethodName VARCHAR(500),
	CatchMethodSignature VARCHAR(500),
	CatchClassName VARCHAR(500),
	CatchLineNumber INT,
	StackFrameCount INT,
	Stack TEXT,
	PRIMARY KEY (ExceptionId)
)ENGINE=MyISAM;