#!/bin/bash


declare -A type_map

# NOT NULLABLES

# Integer types
typemap["mysqlpp::sql_tinyint"]="TINYINT NOT NULL"
typemap["mysqlpp::sql_tinyint_unsigned"]="TINYINT UNSIGNED NOT NULL"
typemap["mysqlpp::sql_smallint"]="SMALLINT NOT NULL"
typemap["mysqlpp::sql_smallint_unsigned"]="SMALLINT UNSIGNED NOT NULL"
typemap["mysqlpp::sql_mediumint"]="MEDIUMINT NOT NULL"
typemap["mysqlpp::sql_mediumint_unsigned"]="MEDIUMINT UNSIGNED NOT NULL"
typemap["mysqlpp::sql_int"]="INT NOT NULL"
typemap["mysqlpp::sql_int_unsigned"]="INT UNSIGNED NOT NULL"
typemap["mysqlpp::sql_bigint"]="BIGINT NOT NULL"
typemap["mysqlpp::sql_bigint_unsigned"]="BIGINT UNSIGNED NOT NULL"
typemap["mysqlpp::sql_bool"]="BOOL NOT NULL"

# Floating type
typemap["mysqlpp::sql_float"]="FLOAT NOT NULL"
typemap["mysqlpp::sql_double"]="DOUBLE NOT NULL"
typemap["mysqlpp::sql_decimal"]="NUMERIC NOT NULL"
typemap["mysqlpp::sql_numeric"]="NUMERIC NOT NULL"
typemap["mysqlpp::sql_fixed"]="NUMERIC NOT NULL"
typemap["mysqlpp::sql_float4"]="NUMERIC NOT NULL"
typemap["mysqlpp::sql_float8"]="NUMERIC NOT NULL"

# BLOB types

typemap["mysqlpp::sql_date"]="DATE NOT NULL"
typemap["mysqlpp::sql_time"]="TIME NOT NULL"
typemap["mysqlpp::sql_datetime"]="DATETIME NOT NULL"
typemap["mysqlpp::sql_timestamp"]="TIMESTAMP NOT NULL"

# ENUM / SET

typemap["mysqlpp::sql_enum"]="ENUM(...) NOT NULL"
typemap["mysqlpp::sql_set"]="SET(...) NOT NULL"


# NULLABLES

# Integer types
typemap["mysqlpp::sql_tinyint_null"]="TINYINT"
typemap["mysqlpp::sql_tinyint_unsigned_null"]="TINYINT UNSIGNED"
typemap["mysqlpp::sql_smallint_null"]="SMALLINT"
typemap["mysqlpp::sql_smallint_unsigned_null"]="SMALLINT UNSIGNED"
typemap["mysqlpp::sql_mediumint_null"]="MEDIUMINT"
typemap["mysqlpp::sql_mediumint_unsigned_null"]="MEDIUMINT UNSIGNED"
typemap["mysqlpp::sql_int_null"]="INT"
typemap["mysqlpp::sql_int_unsigned_null"]="INT UNSIGNED"
typemap["mysqlpp::sql_bigint_null"]="BIGINT"
typemap["mysqlpp::sql_bigint_unsigned_null"]="BIGINT UNSIGNED"
typemap["mysqlpp::sql_bool_null"]="BOOL"

# Floating type
typemap["mysqlpp::sql_float_null"]="FLOAT"
typemap["mysqlpp::sql_double_null"]="DOUBLE"
typemap["mysqlpp::sql_decimal_null"]="NUMERIC"
typemap["mysqlpp::sql_numeric_null"]="NUMERIC"
typemap["mysqlpp::sql_fixed_null"]="NUMERIC"
typemap["mysqlpp::sql_float4_null"]="NUMERIC"
typemap["mysqlpp::sql_float8_null"]="NUMERIC"

# BLOB types

typemap["mysqlpp::sql_date_null"]="DATE"
typemap["mysqlpp::sql_time_null"]="TIME"
typemap["mysqlpp::sql_datetime_null"]="DATETIME"
typemap["mysqlpp::sql_timestamp_null"]="TIMESTAMP"

# ENUM / SET

typemap["mysqlpp::sql_enum_null"]="ENUM(...)"
typemap["mysqlpp::sql_set_null"]="SET(...)"

