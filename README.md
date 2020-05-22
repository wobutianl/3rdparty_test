<!--
 * @Description: 
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-14 09:40:12
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-14 09:56:18
 -->
# 3rdparty_test

test for 3rdparty use

## depend

boost
c++11

## sqlite3

sqlite3 storage in binary

## cereal

serialize class into binary
and unserialize it into class
but with low speed

## smartdb

sqlite3 client
convinent in use than sqlite.h

## yas

c++ struct serialize
can storage it into sqlite with binary but have some problem .
more efficent than cereal

## Mat

convert pointer memory into CV::Mat

## flatbuffer(need install first)

c++ serialze library
fast in undecode

## 


## 
qgis label:
1: 
'id:' + tostring(id) + if(left , 'left:' + tostring(left) , '') + if(right,  'right:' + tostring(right), '' )  + if(del, 'del:'+tostring(del), '')
2: 

Map Position: 于田南路

attribute:

color:
-1: 未知
0: 白线
1: 黄线
2: 边缘

type:
-1: 边沿 (路牙)
0: 虚线
1: 实线

parent:


- make sequence dependence
1. line direction with right point seq
2. line connection (optional)
3. cut particular situation line

- may exists question

/*situation1*/
--      
--  --
------
in such situation: one have three seq, and another have just two.
how to set seq ? 

Answer:
must be cut into two part.


/*situation2*/
----->
----->
<-----
<-----
----->
----->

a road have third direction part, code can not deal with such situation
