const STATUS = {
    OK: 200,
    ACCEPTED: 202,
    BAD_REQUEST: 400,
    NOT_FOUND: 404,
    INTERNAL_SERVER_ERROR: 500
};

const btnUpVisitor = document.getElementById('up-visitor');
const btnDownVisitor = document.getElementById('down-visitor');
const btnUpLocal = document.getElementById('up-local');
const btnDownLocal = document.getElementById('down-local');
const btnUpChuker = document.getElementById('up-chuker');
const btnDownChuker = document.getElementById('down-chuker');
const btnStartTimer = document.getElementById('start-timer');
const btnStopTimer = document.getElementById('stop-timer');
const btnResetTimer = document.getElementById('reset-timer');
const btnResetAll = document.getElementById('reset-all');
const visitorValue = document.getElementById('visitor');
const localValue = document.getElementById('local');
const chukerValue = document.getElementById('chuker');
const timerMMValue = document.getElementById('timer-mm');
const timerSSValue = document.getElementById('timer-ss');
const IP = "192.168.4.1";

const command = {
    INC_SCORE_VISITOR: 0,
    INC_SCORE_LOCAL: 1,
    DEC_SCORE_VISITOR: 2,
    DEC_SCORE_LOCAL: 3,
    INC_CHUKER: 4,
    DEC_CHUKER: 5,
    START_TIMER: 6,
    STOP_TIMER: 7,
    RESET_TIMER: 8,
    RESET_ALL: 9
};

const dataIndex = {
    VISITOR: 0,
    LOCAL: 1,
    CHUKER: 2,
    TIMER_MM: 3,
    TIMER_SS: 4,
}

/* -------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------ SCOREBOARD -------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */
// Visitor
btnUpVisitor.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_VISITOR}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnDownVisitor.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_VISITOR}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

// Local
btnUpLocal.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_LOCAL}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnDownLocal.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_LOCAL}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

// Chuker
btnUpChuker.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/chuker?cmd=${command.INC_CHUKER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnDownChuker.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/chuker?cmd=${command.DEC_CHUKER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

// Timer
btnStartTimer.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.START_TIMER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnStopTimer.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.STOP_TIMER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnResetTimer.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.RESET_TIMER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

btnResetAll.addEventListener('click', async() => {
    const rawResponse = await fetch(`http://${IP}/reset`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
});

/* -------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------ INIT VALUES ------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */
// Al cargarse el sitio web, buscar datos del tablero.
window.addEventListener('load', async () => {
    const rawResponse = await fetch(`http://${IP}/scoreboard`, {
        headers: {
            'Content-Type': 'text/plain'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.OK) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

/* -------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------- UTILITIES -------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */
function setScoreboardValues(dataString){
    const data = dataString.split(',');
    visitorValue.value = data[dataIndex.VISITOR];
    localValue.value = data[dataIndex.LOCAL];
    chukerValue.value = data[dataIndex.CHUKER];
    timerMMValue.value = data[dataIndex.TIMER_MM];
    timerSSValue.value = data[dataIndex.TIMER_SS];
}

setInterval(async() => {
    const rawResponse = await fetch(`http://${IP}/scoreboard`, {
        headers: {
            'Content-Type': 'text/plain'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.OK) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
}, 500);