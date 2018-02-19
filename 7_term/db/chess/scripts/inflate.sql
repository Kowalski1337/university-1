-- The data is generated by some random generator.
-- Any resemblance to actual persons, living or dead, is purely coincidental.

insert into Users (user_name, email, password, user_country) values
    ('Artem',       'a@g.c',            1,  'UA'),
    ('Radimir',     'r@g.c',            2,  'RU'),
    ('Ivan',        'v@g.c',            3,  'BY'),
    ('Magnus',      'm@g.c',            4,  'NO'),
    ('Sophia',      'Sophia@g.c',       5,  'US'),
    ('Jackson',     'Jackson@g.c',      6,  'US'),
    ('Olivia',      'Olivia@g.c',       7,  'US'),
    ('Liam',        'Liam@g.c',         8,  'US'),
    ('Emma',        'Emma@g.c',         9,  'US'),
    ('Noah',        'Noah@g.c',         10, 'US');

insert into Puzzle (puzzle_record, complexity)
    select '{}', i from generate_series(1300, 1700, 5) as t(i);

insert into Tournament
(   tournament_name
,   game_variant   
,   rating_type          
,   time_control   
,   start_time
,   duration
,   admin_id
) values
    ('First tournament', 'classical', 'blitz', ROW('fischer', ROW(3, 0)),
        current_timestamp, interval '1 hour', 1);

insert into TournamentParticipation (tournament_id, user_id)
    select 1, i from generate_series(1, 5) as t(i);

create or replace function PlayRandomGames()
returns void
as $$
declare
    r           double precision;
    rated       boolean;
    game_result GameResult;
    white_id    int;
    black_id    int;
    rating_type RatingType;
    game_var    GameVariant;
begin
    for i in 1..1000 loop
        if random() < 0.1 then
            rated = false;
        else
            rated = true;
        end if;

        r = random();
        if r < 0.4 then
            game_result = 'white';
        elseif r < 0.75 then
            game_result = 'black';
        else
            game_result = 'draw';
        end if;

        r = random();
        game_var = 'classical';
        if r < 0.7 then
            rating_type = 'blitz';
        elseif r < 0.8 then
            rating_type = 'blitz';
        elseif r < 0.9 then
            rating_type = 'rapid';
        else
            rating_type = 'chess960';
            game_var = 'chess960';
        end if;

        white_id = cast(random() * 10 + 0.5 as int);
        loop
            black_id = cast(random() * 10 + 0.5 as int);
            exit when black_id != white_id;
        end loop;

        insert into Game
        (game_variant, rating_type, time_control, game_result,
                start_time, duration, game_record, white_id, black_id) values
            (game_var, case when rated then rating_type else null end, ROW('fischer', ROW(3, 0)), game_result,
                current_timestamp + interval '0.001 second' * i, interval '0.00001 second',
                '{}', white_id, black_id);

        if game_var = 'classical' and rated and rating_type = 'blitz' and
                white_id <= 5 and black_id <= 5 and random() < 0.3 then
            insert into TournamentGame (tournament_id, game_id)
                values (1, i);
        end if;


        if random() < 0.05 then
            insert into Follows values
                (white_id, black_id)
            on conflict do nothing;
        end if;


        white_id = cast(random() * 81 + 0.5 as int);
        if random() < 0.5 then
            rated = true;
        else
            rated = false;
        end if;
        insert into PuzzleAttempt values
            (white_id, black_id, rated, current_timestamp + interval '0.001 second' * i);
    end loop;
end;
$$ language plpgsql;

select PlayRandomGames();