container = document.querySelector(".container");
button = document.querySelector(".button");
button.addEventListener("click", function(){
    if (button.textContent === "Pause") {
        button.textContent = "Play";
    } else {
        button.textContent = "Pause";
    }
})

