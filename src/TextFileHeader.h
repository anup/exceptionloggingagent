/* TextFileHeader.h
 *
 * Include file for the Text Logging.
 *
 * Author: <anup.k@directi.com>
 *
 * Copyright (C) 2008 Directi
 *
 * This file is part of ExceptionLoggingAgent.
 *
 * ExceptionLoggingAgent is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * ExceptionLoggingAgent is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTFILEHEADER_H_
#define TEXTFILEHEADER_H_

#include "header.h"
#include "QueueThreadHeader.h"

#define MAX_WRITE_BUFFER_LENGTH 65536
#define MAX_READ_BUFFER_LENGTH 65536

#define WRITE_MODE "a"
#define READ_MODE "r"

#define FIELD_DELIMITER "~"

#endif /* TEXTFILEHEADER_H_ */
