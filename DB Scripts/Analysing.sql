USE exception;

CREATE TABLE tbl_ExceptionMasterLog
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

CREATE TABLE tbl_ExceptionMaster
(
	ExceptionId INT NOT NULL AUTO_INCREMENT,
	Exception TEXT,
	INDEX (Exception(767)),
	Count INT,
	NewException BIT DEFAULT 1,
	PRIMARY KEY (ExceptionId)	
)ENGINE=MyISAM;

CREATE TABLE tbl_MappingExceptionOccurrence
(
	MasterExceptionId INT NOT NULL,
	INDEX (MasterExceptionId),
	FOREIGN KEY (MasterExceptionId) REFERENCES tbl_ExceptionMaster(ExceptionId) ON UPDATE CASCADE ON DELETE RESTRICT,
	OccurrenceExceptionId INT NOT NULL,
	INDEX (OccurrenceExceptionId),
	FOREIGN KEY (OccurrenceExceptionId) REFERENCES tbl_ExceptionMasterLog(ExceptionId) ON UPDATE CASCADE ON DELETE RESTRICT	
)ENGINE=MyISAM;

DELIMITER ~
CREATE
    TRIGGER UpdateExceptionMaster BEFORE INSERT ON tbl_ExceptionMasterLog 
    FOR EACH ROW BEGIN
    	DECLARE strException TEXT;
    	DECLARE recCount INT;
    	DECLARE excpIdInMaster INT;
    	
    	
    	SET strException = TRIM(CONCAT_WS('|', 
			SUBSTRING(NEW.ExceptionMsg, 1, CHAR_LENGTH(NEW.ExceptionMsg) - 1), 
			SUBSTRING(NEW.ExceptionType, 1, CHAR_LENGTH(NEW.ExceptionType) - 1), 
			SUBSTRING(NEW.SourceFileName, 1, CHAR_LENGTH(NEW.SourceFileName) - 1), 
			SUBSTRING(NEW.CatchMethodName, 1, CHAR_LENGTH(NEW.CatchMethodName) - 1), 
			SUBSTRING(NEW.CatchMethodSignature, 1, CHAR_LENGTH(NEW.CatchMethodSignature) - 1), 
			SUBSTRING(NEW.CatchClassName, 1, CHAR_LENGTH(NEW.CatchClassName) - 1), 
			NEW.CatchLineNumber, NEW.StackFrameCount, SUBSTRING(NEW.Stack, 1, CHAR_LENGTH(NEW.Stack) - 1))
			);
			
		SET recCount = 0;

		SELECT COUNT(*) INTO recCount FROM tbl_ExceptionMaster WHERE Exception = strException;   	

		IF recCount > 0 THEN
			UPDATE tbl_ExceptionMaster SET Count = Count + 1 WHERE Exception = strException;
		ELSE
			INSERT INTO tbl_ExceptionMaster (Exception, Count, NewException) VALUES (strException, 1, 1);
		END IF;	
		
		SELECT ExceptionId INTO excpIdInMaster FROM tbl_ExceptionMaster WHERE Exception = strException;
		
		INSERT INTO tbl_MappingExceptionOccurrence VALUES (excpIdInMaster, NEW.ExceptionId);
		
	END; 
~