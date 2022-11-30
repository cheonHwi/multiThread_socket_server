CREATE TABLE user_data( 
uuid VARCHAR(36) DEFAULT (UUID()) , 
user_id varchar(16) not null unique key,
user_pw varchar(16) not null,
constraint user_data_primary_key primary key(uuid)
);

CREATE TABLE chat_room(
	room_number int auto_increment,
	room_name varchar(20) not null,
    constraint room_pk primary key(room_number)
);
    
CREATE TABLE chat_content(
	chat_index int primary key auto_increment,
	chat_writer varchar(15),
	chat_content varchar(1024),
	chat_room int not null,
	chat_time varchar(25) not null,
	foreign key (chat_room) references chat_room(room_number) on update cascade on delete cascade
);

drop table user_data;
drop table chat_room;
drop table chat_content;

select * from user_data;
select * from chat_room;
select * from chat_content;

select * from user_data u, chat_room r, chat_content c
where c.chat_writer = u.user_id;

select * from chat_content order by chat_index desc limit 10;
select chat_writer, chat_content, chat_time from (select * from clang.chat_content where chat_room = 1 order by chat_index desc limit 10) as a order by chat_index asc;
select room_number, room_name from chat_room where room_member = 'test';


insert into user_data(user_id, user_pw) values('test', '1234');
insert into chat_room(room_name) values('default_room');
insert into chat_room(room_name) values('second_room');
insert into chat_room(room_name) values 
insert into chat_content(chat_writer, chat_content, chat_time) values('%s', '%s','%s');
