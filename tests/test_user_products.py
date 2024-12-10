import pytest
import aiohttp
import logging

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


@pytest.fixture
async def auth_headers(service_client):
    data = {
        "username": "test_user",
        "password": "test_password"
    }
    response = await service_client.post(
        '/register',
        headers={'Content-Type': "application/json"},
        json=data
    )
    assert response.status == 200

    response = await service_client.post(
        '/login',
        headers={'Content-Type': "application/json"},
        json=data
    )
    assert response.status == 200

    response_data = response.json()
    token = response_data['id']

    return {"X-Ya-User-Ticket": f"{token}"}


@pytest.fixture
async def create_all_items(service_client, auth_headers):
    data = {
        "name": "test_room"
    }
    room_response = await service_client.post(
        '/v1/rooms',
        headers=auth_headers,
        json=data
    )
    assert room_response.status == 200
    data = {
        "name": "test_product",
        "price": 12000,
        "room_id": 1
    }
    product_response = await service_client.post(
        '/v1/products',
        headers=auth_headers,
        json=data
    )
    assert product_response.status == 200
    data = {
        "product_id": 1,
        "user_id": 1
    }
    user_product_response = await service_client.post(
        '/v1/user-products',
        headers=auth_headers,
        json=data
    )
    assert user_product_response.status == 200
    return auth_headers


@pytest.fixture
async def setup_room(service_client, auth_headers):
    data = {
        "name": "test_room"
    }
    response = await service_client.post(
        '/v1/rooms',
        headers=auth_headers,
        json=data
    )
    assert response.status == 200
    return auth_headers


@pytest.fixture
async def setup_product(service_client, setup_room):
    data = {
        "name": "test_product",
        "price": 12000,
        "room_id": 1
    }
    response = await service_client.post(
        '/v1/products',
        headers=setup_room,
        json=data
    )
    assert response.status == 200
    return setup_room


@pytest.fixture
async def create_user_product_headers(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 1
    }
    response = await service_client.post(
        '/v1/user-products',
        headers=setup_product,
        json=data
    )
    assert response.status == 200
    return setup_product


@pytest.mark.asyncio
async def test_add_user_to_product_default_status(service_client, setup_product):
    data = {
        "user_id": 1,
        "product_id": 1,
        "status": "UNPAID"
    }

    response = await service_client.post(
        "/v1/user-products",
        headers=setup_product,
        json=data
    )
    assert response.status == 200
    response_data = response.json()

    assert response_data["id"] is not None
    assert response_data["status"] == "UNPAID"
    assert response_data["user_id"] == 1
    assert response_data["product_id"] == 1


@pytest.mark.asyncio
async def test_add_user_to_product_valid(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 1,
        "status": "PAID"
    }
    response = await service_client.post(
        '/v1/user-products',
        headers=setup_product,
        json=data
    )
    assert response.status == 200
    response_data = response.json()
    assert response_data["product_id"] == data["product_id"]
    assert response_data["user_id"] == data["user_id"]
    assert response_data["status"] == data["status"]


@pytest.mark.asyncio
async def test_add_user_to_product_missing_fields(service_client, setup_product):
    data = {
        "product_id": 1
    }
    response = await service_client.post('/v1/user-products', headers=setup_product, json=data)
    assert response.status == 400
    response_data = response.json()
    assert response_data["error"] == "'product_id' and 'user_id' fields are required"
@pytest.mark.asyncio
async def test_add_user_to_product_invalid_product_id(service_client, setup_product):
    data = {
        "product_id": 99999,
        "user_id": 1
    }
    response = await service_client.post(
        '/v1/user-products',
        headers=setup_product,
        json=data)
    assert response.status == 404
    response_data = response.json()
    assert response_data["error"] == "Product Does not exist"
@pytest.mark.asyncio
async def test_add_user_to_product_invalid_user_id(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 99999
    }
    response = await service_client.post('/v1/user-products', headers=setup_product, json=data)
    assert response.status == 404
    response_data = response.json()
    assert response_data["error"] == "User Does not exist!"
@pytest.mark.asyncio
async def test_add_user_to_product_duplicate_entry(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 1
    }
    # First attempt
    response = await service_client.post('/v1/user-products', headers=setup_product, json=data)
    assert response.status == 200

    # Duplicate attempt
    response = await service_client.post('/v1/user-products', headers=setup_product, json=data)
    assert response.status == 409
    response_data = response.json()
    assert response_data["error"] == "User already associated with this product"
@pytest.mark.asyncio
async def test_add_user_to_product_invalid_status(service_client, setup_product):
    data = {
        "product_id": 1,
        "user_id": 1,
        "status": "INVALID_STATUS"
    }
    response = await service_client.post('/v1/user-products', headers=setup_product, json=data)
    assert response.status == 400
    response_data = response.json()
    assert "error" in response_data


@pytest.mark.asyncio
async def test_get_user_products_by_id(service_client, create_user_product_headers):
    user_id = 1

    response = await service_client.get(
        f"/v1/user-products/{user_id}",
        headers=create_user_product_headers
    )
    assert response.status == 200
    response_data = response.json()

    assert "items" in response_data


@pytest.mark.asyncio
async def test_get_all_user_products(service_client, create_all_items):
    response = await service_client.get(
        "/v1/user-products/?room_id=1",
        headers=create_all_items
    )
    assert response.status == 200
    response_data = response.json()

    for item in response_data["users"]:
        assert "user_id" in item
        assert "product_ids" in item


@pytest.mark.asyncio
async def test_update_user_product(service_client, create_all_items):
    user_product_id = 1
    data = {
        "status": "PAID"
    }

    response = await service_client.put(
        f"/v1/user-products/{user_product_id}",
        headers=create_all_items,
        json=data
    )
    assert response.status == 200
    response_data = response.json()

    assert response_data["id"] == user_product_id
    assert response_data["status"] == "PAID"


@pytest.mark.asyncio
async def test_invalid_user_product_update(service_client, create_all_items):
    user_product_id = 999
    data = {
        "status": "PAID"
    }

    response = await service_client.put(
        f"/v1/user-products/{user_product_id}",
        headers=create_all_items,
        json=data
    )
    assert response.status == 404  # Assuming non-existent IDs return 404
    response_data = response.json()

    assert "error" in response_data
