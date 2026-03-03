async function refresh_session()
{
    const response = await fetch('/try_login', {
        method: 'GET',
        headers: {
            'Content-Type': 'application/json'
        },
    });

    if (response.ok) {
        chat_mode();
    }
    else
    {
        login_or_register_mode();
    }
}

async function login_or_register_mode() {
    const login_or_register = document.getElementById('login_or_register_container');
    const chat = document.getElementById('chat_container');

    login_or_register.hidden = false;
    chat.hidden = true;
}

async function send_message()
{
    const input = document.getElementById('message_input').value;
    await fetch('/send_message', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            input: input,
        })
    });
}
function render_messages(messages)
{
    const message_list = document.getElementById('message_list');
    message_list.innerHTML = messages;    
}
function fetch_messages() {
    fetch('/get_messages')
        .then(response => {
            if (!response.ok)
            {
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .then(messages => {
            render_messages(messages);
        })
        .catch(error => {
            console.error('Error fetching messages:', error);
        });
}


const pollingInterval = 500;
let pollTimer;
function start_polling()
{
    fetch_messages();
    pollTimer = setInterval(fetch_messages, pollingInterval);
}

function stop_polling()
{
    clearInterval(pollTimer);
    is_polling = false;
}

var is_polling = false;

async function chat_mode()
{
    if (!is_polling) { is_polling = true; start_polling(); }

    const login_or_register = document.getElementById('login_or_register_container');
    const chat = document.getElementById('chat_container');

    login_or_register.hidden = true;
    chat.hidden = false;
}

async function submit_login() {
    const username_input = document.getElementById('login_username').value;
    const password_input = document.getElementById('login_password').value;
    const feedback = document.getElementById('login_feedback');

    const response = await fetch('/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            username: username_input,
            password: password_input
        })
    });

    const data = await response.json();

    if (response.ok)
    {
        chat_mode();
        document.getElementById('login_username').value = "";
        document.getElementById('login_password').value = "";
        feedback.innerText = ""; 
    }
    else if (response.status === 404)
    {
        feedback.innerText = data.error; 
    }
    else if (response.status === 401)
    {
        document.getElementById('login_password').value = "";
        feedback.innerText = data.error; 
    }
    else
    {
        feedback.innerText = "An unexpected error occurred.";
    }
}

async function logout()
{
    await fetch('/logout', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
    });

    stop_polling();
    login_or_register_mode();
}

async function submit_register() {
    const username_input = document.getElementById('register_username').value;
    const password_input = document.getElementById('register_password').value;
    const feedback = document.getElementById('register_feedback');

    const response = await fetch('/register', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            username: username_input,
            password: password_input
        })
    });

    const data = await response.json();

    if (response.ok)
    {
        chat_mode();
        document.getElementById('register_username').value = "";
        document.getElementById('register_password').value = "";
        feedback.innerText = ""; 
    }
    else if (response.status === 409)
    {
        document.getElementById('register_password').value = ""; 
        feedback.innerText = data.error; 
    }
    else
    {
        feedback.innerText = "Registration failed. Please try again.";
    }
}