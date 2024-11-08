let currentPage = 1;
const postsPerPage = 10;
let allPosts = [];

async function fetchPosts() {
    const response = await fetch('/posts');
    allPosts = await response.json();
    displayPosts();
}

function displayPosts() {
    const postsContainer = document.getElementById('posts');
    const postsMap = {};
    postsContainer.innerHTML = '';

    // Create a map of posts by id
    allPosts.forEach(post => {
        postsMap[post.id] = post;
        post.replies = [];
    });

    // Populate replies array for each post
    allPosts.forEach(post => {
        if (post.replyTo && postsMap[post.replyTo]) {
            postsMap[post.replyTo].replies.push(post);
        }
    });

    // Get the posts for the current page
    const topPosts = Object.values(postsMap).filter(post => !post.replyTo);
    const totalPages = Math.ceil(topPosts.length / postsPerPage);
    const start = (currentPage - 1) * postsPerPage;
    const end = start + postsPerPage;
    const paginatedPosts = topPosts.slice(start, end);

    // Append paginated top-level posts to the container
    paginatedPosts.forEach(post => {
        postsContainer.appendChild(createPostElement(post, postsMap));
    });

    // Update pagination controls
    const pagination = document.getElementById('pagination');
    pagination.innerHTML = '';
    
    if (currentPage > 1) {
        const prevButton = document.createElement('button');
        prevButton.textContent = `${currentPage - 1}`;
        prevButton.onclick = () => changePage(currentPage - 1);
        pagination.appendChild(prevButton);
    }

    const currentButton = document.createElement('button');
    currentButton.textContent = `${currentPage}`;
    currentButton.className = 'current';
    pagination.appendChild(currentButton);

    if (currentPage < totalPages) {
        const nextButton = document.createElement('button');
        nextButton.textContent = `${currentPage + 1}`;
        nextButton.onclick = () => changePage(currentPage + 1);
        pagination.appendChild(nextButton);
    }
}

function createPostElement(post, postsMap) {
    const postElement = document.createElement('div');
    postElement.className = 'post';
    postElement.innerHTML = `
        <p>αρ. δημ.: <a href="#" onclick="replyTo('${post.id}')">${post.id}</a> | ${post.timestamp}</p>
        <p>Απάντηση σε: ${post.replyTo ? post.replyTo : 'Ουδέν'}</p>
        <img src="../uploads/${post.imagePath}" alt="Image" style="max-width: 50%; height: auto;" onclick="openImageInNewTab('${post.imagePath}')">
        <p>${post.content}</p>
        ${post.replies.length > 0 ? `<p class="reply-toggle">Εμφάνιση νήματος (${post.replies.length})</p>` : ''}
        <div class="reply-container"></div>
    `;
    const replyContainer = postElement.querySelector('.reply-container');
    post.replies.forEach(reply => {
        replyContainer.appendChild(createPostElement(reply, postsMap));
    });
    const replyToggle = postElement.querySelector('.reply-toggle');
    if (replyToggle) {
        replyToggle.addEventListener('click', () => {
            const isVisible = replyContainer.style.display === 'block';
            replyContainer.style.display = isVisible ? 'none' : 'block';
            replyToggle.textContent = isVisible ? `Εμφάνιση νήματος (${post.replies.length})` : `Απόκρυψη νήματος (${post.replies.length})`;
        });
    }
    return postElement;
}

function replyTo(id) {
    const replyToField = document.getElementById('reply_to');
    replyToField.value = id;
}

function openImageInNewTab(imagePath) {
    const win = window.open(`../uploads/${imagePath}`, '_blank');
    win.focus();
}

function scrollToTop() {
    window.scrollTo({ top: 0, behavior: 'smooth' });
}

function changePage(page) {
    currentPage = page;
    displayPosts();
}

window.onload = fetchPosts;

    // Array of image URLs
    const images = [
        'img/rand/1.jpg',
        'img/rand/2.jpeg',
        'img/rand/3.jpeg',
        'img/rand/4.jpg',
        // Add more image URLs as needed
    ];

    // Function to select a random image
    function getRandomImage() {
        const randomIndex = Math.floor(Math.random() * images.length);
        return images[randomIndex];
    }

    // Set the image source on page load
    document.getElementById('randomImage').src = getRandomImage();