const STATUS = {
    OK: 200,
    ACCEPTED: 202,
    BAD_REQUEST: 400,
    NOT_FOUND: 404,
    INTERNAL_SERVER_ERROR: 500
};
const timerStatus = {
    STOPPED: 0,
    RUNNING: 1,
    FINISHED: 2
}

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
const btnUpMinute = document.getElementById('up-minute');
const btnDownMinute = document.getElementById('down-minute');
const btnUpSecond = document.getElementById('up-second');
const btnDownSecond = document.getElementById('down-second');
const IP = "192.168.4.1";
const RUNNING = 1; // estado para timer activo
let refreshTimer = undefined;
let timerState = timerStatus.STOPPED;

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
    TIMER_STATE: 5
}

/* -------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------ SCOREBOARD -------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */
// Visitor
btnUpVisitor.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_VISITOR}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownVisitor.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_VISITOR}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// Local
btnUpLocal.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.INC_SCORE_LOCAL}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownLocal.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/score?cmd=${command.DEC_SCORE_LOCAL}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// Chuker
btnUpChuker.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/chuker?cmd=${command.INC_CHUKER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

btnDownChuker.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/chuker?cmd=${command.DEC_CHUKER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
    }
});

// Timer
btnStartTimer.addEventListener('click', async () => {
    if (timerState === timerStatus.STOPPED) {
        const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.START_TIMER}`, {
            headers: {
                'Content-Type': 'text/css'
            },
            method: 'GET'
        });

        if (rawResponse.status === STATUS.ACCEPTED) {
            // Se crea intervalo para recuperar datos de tablero cada 500ms
            refreshTimer = setInterval(async () => {
                const innerRawResponse = await fetch(`http://${IP}/scoreboard`, {
                    headers: {
                        'Content-Type': 'text/css'
                    },
                    method: 'GET'
                });
                const response = await innerRawResponse.text();
                setScoreboardValues(response);
                setOptions();
            }, 200);
            timerState = timerStatus.RUNNING;
        }
    }
});

btnStopTimer.addEventListener('click', async () => {
    if (timerState === timerStatus.RUNNING) {
        const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.STOP_TIMER}`, {
            headers: {
                'Content-Type': 'text/css'
            },
            method: 'GET'
        });
        if (rawResponse.status === STATUS.ACCEPTED) {
            clearInterval(refreshTimer);
            timerState = timerStatus.STOPPED;
            setOptions();
        }
    }
});

btnResetTimer.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/timer?cmd=${command.RESET_TIMER}`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        clearInterval(refreshTimer);
        timerState = timerStatus.STOPPED;
        setOptions();
    }
});

btnUpMinute.addEventListener('click', async() => {
    if(timerMMValue.value < 99){
        timerMMValue.value = (parseInt(timerMMValue.value) + 1).toString().padStart(2,'0');
        sendTimerData();
    }    
});

btnDownMinute.addEventListener('click', async() => {
    if(timerMMValue.value > 0){
        timerMMValue.value = (parseInt(timerMMValue.value) - 1).toString().padStart(2,'0');
        sendTimerData();
    }
});

btnUpSecond.addEventListener('click', async() => {
    if(timerSSValue.value < 99){
        timerSSValue.value = (parseInt(timerSSValue.value) + 1).toString().padStart(2,'0');
        sendTimerData();
    }
});

btnDownSecond.addEventListener('click', async() => {
    if(timerSSValue.value > 0){
        timerSSValue.value = (parseInt(timerSSValue.value) - 1).toString().padStart(2,'0');
        sendTimerData();
    }
});

// Reset all
btnResetAll.addEventListener('click', async () => {
    const rawResponse = await fetch(`http://${IP}/reset`, {
        headers: {
            'Content-Type': 'text/css'
        },
        method: 'GET'
    });
    if (rawResponse.status === STATUS.ACCEPTED) {
        const response = await rawResponse.text();
        setScoreboardValues(response);
        setOptions();
        clearInterval(refreshTimer);
    }
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
        setOptions();
    }
});

/* -------------------------------------------------------------------------------------------------------------- */
/* ------------------------------------------------- UTILITIES -------------------------------------------------- */
/* -------------------------------------------------------------------------------------------------------------- */
function setScoreboardValues(dataString) {
    const data = dataString.split(',');
    visitorValue.value = data[dataIndex.VISITOR].padStart(2,'0');
    localValue.value = data[dataIndex.LOCAL].padStart(2,'0');
    chukerValue.value = data[dataIndex.CHUKER];
    timerMMValue.value = data[dataIndex.TIMER_MM].padStart(2,'0');
    timerSSValue.value = data[dataIndex.TIMER_SS].padStart(2,'0');
    timerState = parseInt(data[dataIndex.TIMER_STATE]);
}

async function sendTimerData(){
    const rawResponse = await fetch(`http://${IP}/timer?mm=${timerMMValue.value}&ss=${timerSSValue.value}`, {
        headers: {
            'Content-Type': 'text/plain',
        },
        method: 'POST',
    });
    console.log("Data sent!");
    if(rawResponse.status !== STATUS.ACCEPTED){ console.log("Han error has ocurred updating the timer");}
}

function setOptions(){
    console.log("timerState", timerState);
    if(timerState === timerStatus.STOPPED){
        btnStopTimer.disabled = true;
        btnStartTimer.disabled = false;
        btnUpMinute.disabled = false;
        btnDownMinute.disabled = false;
        btnUpSecond.disabled = false;
        btnDownSecond.disabled = false;
    }
    else if(timerState === timerStatus.RUNNING){
        btnStopTimer.disabled = false;
        btnStartTimer.disabled = true;
        btnUpMinute.disabled = true;
        btnDownMinute.disabled = true;
        btnUpSecond.disabled = true;
        btnDownSecond.disabled = true;
    }
    else{
        btnStopTimer.disabled = true;
        btnStartTimer.disabled = true;
        btnUpMinute.disabled = true;
        btnDownMinute.disabled = true;
        btnUpSecond.disabled = true;
        btnDownSecond.disabled = true;
    }
}