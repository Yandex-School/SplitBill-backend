DROP SCHEMA IF EXISTS public CASCADE;
CREATE SCHEMA IF NOT EXISTS public;

CREATE TYPE room_status AS ENUM (
    'ARCHIVED',
  'ACTIVE'
);

CREATE TYPE paid_status AS ENUM (
  'PAID',
  'UNPAID'
);

CREATE TABLE users
(
    id        serial PRIMARY KEY,
    username  varchar(255) NOT NULL UNIQUE ,
    full_name varchar(255),
    photo     varchar(255),
    password  varchar(255) NOT NULL
);

CREATE TABLE IF NOT EXISTS auth_sessions (
    id serial PRIMARY KEY,
    user_id TEXT NOT NULL,
    foreign key(user_id) REFERENCES users(id)
);

CREATE TABLE rooms
(
    id       serial PRIMARY KEY,
    name     varchar(255) NOT NULL,
    user_id  int4 REFERENCES users(id) ON DELETE CASCADE NOT NULL,
    status   room_status DEFAULT 'ACTIVE'
);

CREATE TABLE products
(
    id      serial PRIMARY KEY,
    name    varchar(255) NOT NULL,
    price   bigint,
    room_id int4 REFERENCES rooms(id) ON DELETE CASCADE NOT NULL
);

CREATE TABLE user_products
(
    id         serial PRIMARY KEY,
    status     paid_status DEFAULT 'UNPAID',
    product_id int4 REFERENCES products(id) ON DELETE CASCADE NOT NULL,
    user_id    int4 REFERENCES users(id) ON DELETE CASCADE NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_user_products_product_id ON user_products (product_id);

CREATE INDEX idx_user_products_user_id ON user_products (user_id);

CREATE INDEX idx_user_products_user_product ON user_products (user_id, product_id);

CREATE INDEX idx_products_room_id ON products (room_id);

CREATE INDEX idx_rooms_owner_id ON rooms (owner_id);

CREATE UNIQUE INDEX idx_users_username ON users (username);

CREATE INDEX idx_user_products_status ON user_products (status);

CREATE INDEX idx_rooms_status ON rooms (status);

CREATE INDEX idx_products_name ON products (name);

CREATE INDEX idx_users_full_name ON users (full_name);