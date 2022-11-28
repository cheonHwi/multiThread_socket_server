select * from user_data;
select * from chat_record;
select * from chating_room;

drop table user_data;
drop table chat_record;
drop table chating_room;


CREATE TABLE user_data( 
uuid VARCHAR(36) DEFAULT (UUID()) , 
user_id varchar(16) not null,
user_pw varchar(16) not null,
constraint user_data_primary_key primary key(uuid, user_id)
);

select * from user_data where user_id='test' and user_pw='1234';

insert into user_data(user_id, user_pw) values( 'test', '1234');

insert into chating_room (room_name, room_member) values('test', 'test');
insert into clang.chat_record(chat_writer, chat_content, room_id) values('test','이번에는 성공하는거 내가 봄',1);
insert into clang.chat_record(chat_writer, chat_content) values('test','test');


insert into chat_record values('test', 'test-message', '대충 지금', 1);

select * from user_data where user_id ='test' and user_pw ='1234';

CREATE TABLE chat_content(
chat_index int primary key auto_increment,
chat_writer varchar(15),
chat_content varchar(1024),
chat_time varchar(25) not null
);

drop table chat_content;

select * from chat_content;

select * from chat_content order by chat_index desc limit 10;
select a.* from (select * from chat_content order by chat_index desc limit 10)a order by a.chat_index;
select * from (select * from clang.chat_content order by chat_index desc limit 10) as a order by chat_index asc;


