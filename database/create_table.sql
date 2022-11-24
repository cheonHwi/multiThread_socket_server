create table user_data(
	user_id varchar(15) not null,
	user_pw varchar(15) not null,
	constraint id_pk primary key (user_id)
);

create table chating_room(
	room_id int auto_increment,
    room_name varchar(10) not null,
	room_member varchar(15),
    constraint room_pk primary key (room_id),
	constraint member_fk foreign key(room_member) references user_data(user_id) on update cascade on delete cascade
);

create table chat_record(
	chat_writer varchar(15) primary key,
    chat_content varchar(255),
    chat_time varchar(25),
    room_id int,
    constraint writer foreign key(chat_writer) references user_data(user_id),
    constraint room foreign key(room_id) references chating_room(room_id) on update cascade on delete cascade
);

drop table chating_room;
drop table user_data;
drop table chat_record;

select * from user_data;
select * from chat_record;
select * from chating_room;