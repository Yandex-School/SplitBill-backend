DROP SCHEMA IF EXISTS public CASCADE;
CREATE SCHEMA IF NOT EXISTS public;

DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM pg_type
        WHERE typname = 'paid_status'
    ) THEN
CREATE TYPE paid_status AS ENUM (
            'PAID',
            'UNPAID'
        );
END IF;
END $$;


CREATE TABLE IF NOT EXISTS users
(
    id        serial4 PRIMARY KEY,
    username  varchar(255) NOT NULL UNIQUE ,
    full_name varchar(255),
    photo_url     varchar(255),
    password  varchar(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS auth_sessions (
    id serial PRIMARY KEY,
    user_id int4 REFERENCES users(id) ON DELETE CASCADE NOT NULL
);

CREATE TABLE IF NOT EXISTS rooms
(
    id       serial4 PRIMARY KEY,
    name     varchar(255) NOT NULL,
    user_id  int4 REFERENCES users(id) ON DELETE CASCADE NOT NULL
);

CREATE TABLE IF NOT EXISTS products
(
    id      serial4 PRIMARY KEY,
    name    varchar(255) NOT NULL,
    price   bigint,
    room_id int4 REFERENCES rooms(id) ON DELETE CASCADE NOT NULL,
    UNIQUE (name, room_id)
);

CREATE TABLE IF NOT EXISTS user_products
(
    id         serial4 PRIMARY KEY,
    status     paid_status DEFAULT 'UNPAID',
    product_id int4 REFERENCES products(id) ON DELETE CASCADE NOT NULL,
    user_id    int4 REFERENCES users(id) ON DELETE CASCADE NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_user_products_product_id ON user_products (product_id);

CREATE INDEX IF NOT EXISTS idx_user_products_user_id ON user_products (user_id);

CREATE INDEX IF NOT EXISTS idx_user_products_user_product ON user_products (user_id, product_id);

CREATE INDEX IF NOT EXISTS idx_products_room_id ON products (room_id);

CREATE INDEX IF NOT EXISTS idx_rooms_user_id ON rooms (user_id);

CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users (username);

CREATE INDEX IF NOT EXISTS idx_user_products_status ON user_products (status);

CREATE INDEX IF NOT EXISTS idx_products_name ON products (name);

CREATE INDEX IF NOT EXISTS idx_users_full_name ON users (full_name);