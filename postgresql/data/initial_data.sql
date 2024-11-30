INSERT INTO public.users (id, username, full_name, photo_url, password)
VALUES
    (1, 'alice', 'Alice Johnson', 'of.com/pics/1235', 'password123'),
    (2, 'bob', 'Bob Brown', 'of.com/pics/1236', 'passw0rd'),
    (3, 'charlie', 'Charlie Davis', 'of.com/pics/1237', 'ch@rlie2023'),
    (4, 'diana', 'Diana Evans', 'of.com/pics/1238', 'diana12345'),
    (5, 'eve', 'Eve Foster', 'of.com/pics/1239', 'securepassword'),
    (6, 'frank', 'Frank Green', 'of.com/pics/1240', 'f1rstPass'),
    (7, 'grace', 'Grace Hall', 'of.com/pics/1241', 'gr@ceH#ll'),
    (8, 'henry', 'Henry Irving', 'of.com/pics/1242', 'henry@789'),
    (9, 'ivy', 'Ivy James', 'of.com/pics/1243', '1vySecure'),
    (10, 'jack', 'Jack Kelly', 'of.com/pics/1244', 'd87068040514f4e921be0dad416e8dbfb5ac8cb2b5d64ce73551ed9900d3e62b') -- 123451678
    ON CONFLICT DO NOTHING;

SELECT setval('users_id_seq', (SELECT MAX(id) FROM users));

INSERT INTO public.rooms (id, name, user_id)
VALUES
    (1, 'free meal', 1),
    (2, 'free meal real', 10),
    (3, 'lesgoooo meal', 3),
    (4, 'obshaga plov', 10)
    ON CONFLICT DO NOTHING;

SELECT setval('rooms_id_seq', (SELECT MAX(id) FROM users));

INSERT INTO public.products (id, name, price, room_id)
VALUES
    (1, 'meat', 10000000, 1),
    (2, 'napitki', 2342342434, 1),
    (3, 'something', 79000000, 1),
    (4, 'meat', 10000000, 2),
    (5, 'napitki2', 2342342434, 2),
    (6, 'something2', 79000000, 3),
    (7, 'meat2', 10000000, 3),
    (8, 'napitki3', 2342342434, 3),
    (9, 'something4', 79000000, 2),
    (10, 'obshaga plov5', 100000000000, 2)
    ON CONFLICT DO NOTHING;

SELECT setval('products_id_seq', (SELECT MAX(id) FROM products));

INSERT INTO public.user_products (id, status, product_id, user_id)
VALUES
    (1, 'PAID', 1, 1),
    (2, 'PAID', 2, 2),
    (3, 'PAID', 3, 3),
    (4, 'UNPAID', 4, 4),
    (5, 'UNPAID', 5, 5),
    (6, 'UNPAID', 6, 6),
    (7, 'UNPAID', 7, 7),
    (8, 'UNPAID', 8, 8),
    (9, 'UNPAID', 9, 9),
    (10, 'PAID', 10, 10)
    ON CONFLICT DO NOTHING;

SELECT setval('user_products_id_seq', (SELECT MAX(id) FROM user_products));