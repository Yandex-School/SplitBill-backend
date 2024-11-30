INSERT INTO public.users (username, full_name, photo, password)
VALUES
    ('alice', 'Alice Johnson', 'of.com/pics/1235', 'password123'),
    ('bob', 'Bob Brown', 'of.com/pics/1236', 'passw0rd'),
    ('charlie', 'Charlie Davis', 'of.com/pics/1237', 'ch@rlie2023'),
    ('diana', 'Diana Evans', 'of.com/pics/1238', 'diana12345'),
    ('eve', 'Eve Foster', 'of.com/pics/1239', 'securepassword'),
    ('frank', 'Frank Green', 'of.com/pics/1240', 'f1rstPass'),
    ('grace', 'Grace Hall', 'of.com/pics/1241', 'gr@ceH#ll'),
    ('henry', 'Henry Irving', 'of.com/pics/1242', 'henry@789'),
    ('ivy', 'Ivy James', 'of.com/pics/1243', '1vySecure'),
    ('jack', 'Jack Kelly', 'of.com/pics/1244', 'jack#pass')
    ON CONFLICT DO NOTHING;

INSERT INTO public.rooms (name, owner_id)
VALUES
    ('free meal', 1),
    ('free meal real', 2),
    ('lesgoooo meal', 3),
    ('obshaga plov', 1)
    ON CONFLICT DO NOTHING;

INSERT INTO public.products (name, price, room_id)
VALUES
    ('meat', 10000000, 1),
    ('napitki', 2342342434, 1),
    ('something', 79000000, 1),
    ('meat', 10000000, 2),
    ('napitki2', 2342342434, 2),
    ('something2', 79000000, 3),
    ('meat2', 10000000, 3),
    ('napitki3', 2342342434, 3),
    ('something4', 79000000, 2),
    ('obshaga plov5', 100000000000, 2)
    ON CONFLICT DO NOTHING;

INSERT INTO public.user_products (status, product_id, user_id)
VALUES
    ('PAID', 1, 1),
    ('PAID', 2, 2),
    ('PAID', 3, 3),
    ('UNPAID', 4, 4),
    ('UNPAID', 5, 5),
    ('UNPAID', 6, 6),
    ('UNPAID', 7, 7),
    ('UNPAID', 8, 8),
    ('UNPAID', 9, 9),
    ('PAID', 10, 10)
    ON CONFLICT DO NOTHING;
