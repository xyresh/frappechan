let currentPage = 1;
const postsPerPage = 10;
let allPosts = [];

async function fetchPosts() {
    const response = await fetch('/posts');
    allPosts = await response.json();
    displayPosts();
}

function displayPosts(selectedPostId = null) {
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

    if (selectedPostId) {
        // Display the single post and all its replies
        const mainPost = postsMap[selectedPostId];
        if (mainPost) {
            const displayThread = (post) => {
                postsContainer.appendChild(createPostElement(post, postsMap));
                post.replies.forEach(reply => displayThread(reply));
            };
            displayThread(mainPost);

            // Add a "Back to All Posts" button
            const backButton = document.createElement('a');
            backButton.textContent = 'Πίσω στην αρχική';
            backButton.className = 'back-button';
            backButton.onclick = () => displayPosts();
            postsContainer.appendChild(backButton);
        }
        return;
    }

    // Separate admin and regular posts
    const adminPosts = Object.values(postsMap).filter(
        post => post.content.startsWith('%admin9669%') && !post.replyTo
    );
    const regularPosts = Object.values(postsMap).filter(
        post => !post.content.startsWith('%admin9669%') && !post.replyTo
    );

    // Sort regular posts
    regularPosts.sort((a, b) => {
        const latestReplyA = a.replies.length > 0 ? a.replies[a.replies.length - 1].timestamp : a.timestamp;
        const latestReplyB = b.replies.length > 0 ? b.replies[b.replies.length - 1].timestamp : b.timestamp;
        return new Date(latestReplyB) - new Date(latestReplyA);
    });

    // Merge and paginate posts
    const sortedPosts = [...adminPosts, ...regularPosts];
    const totalPages = Math.ceil(sortedPosts.length / postsPerPage);
    const start = (currentPage - 1) * postsPerPage;
    const end = start + postsPerPage;
    const paginatedPosts = sortedPosts.slice(start, end);

    // Display paginated posts
    paginatedPosts.forEach(post => {
        const postElement = createPostElement(post, postsMap);

        // Add "View Entire Thread" button
        if (!post.replyTo) {
            const viewThreadButton = document.createElement('a');
            viewThreadButton.textContent = 'Ολόκληρο το Νήμα';
            viewThreadButton.className = 'view-thread-button';
            viewThreadButton.onclick = () => displayPosts(post.id);
            postElement.appendChild(viewThreadButton);
        }

        postsContainer.appendChild(postElement);
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

    // Dynamically replace the path for scaled images
    const scaledImagePath = `../uploads/${post.imagePath}`;
    const fullImagePath = `../uploads/fullsize/${post.imagePath}`;

    // Check if the post starts with %admin9669%
    if (post.content.startsWith('%admin9669%')) {
        post.content = post.content.replace('%admin9669%', '<h3 class="admin-post">ΑΝΤΜΙΝ</h3><br/>');
    }

    // Process content to handle greentext and newlines
    const youtubeRegex = /(?:https?:\/\/)?(?:www\.)?(?:youtube\.com\/watch\?v=|youtu\.be\/)([\w\-]+)/g;
    const parts = post.content.split(youtubeRegex);
    let processedContent = '';

    for (let i = 0; i < parts.length; i++) {
        if (i % 2 === 1) {
            // This part matches a YouTube video ID
            const videoId = parts[i];
            processedContent += `
                <div class="youtube-embed">
                    <iframe 
                        width="300" 
                        height="200" 
                        src="https://www.youtube.com/embed/${videoId}" 
                        frameborder="0" 
                        allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" 
                        allowfullscreen>
                    </iframe>
                </div>
            `;
        } else if (parts[i].trim()) {
            // Process each line separately to handle greentext and newlines
            const lines = parts[i].split('\n');
            lines.forEach(line => {
                if (line.startsWith('>')) {
                    // Apply greentext styling for lines starting with '>'
                    processedContent += `<p class="greentext">&gt;${line.slice(1)}</p>`;
                } else {
                    // Wrap normal text in <p> tags
                    processedContent += `<p>${line}</p>`;
                }
            });
        }
    }

    postElement.innerHTML = `
        <p>αρ. δημ.: <a href="#" onclick="replyTo('${post.id}')">${post.id}</a> | ${post.timestamp}</p>
        <p>Απάντηση σε: ${post.replyTo ? post.replyTo : 'Ουδέν'}</p>
        <img src="${scaledImagePath}" alt="Image" class="post-image" onclick="openFullImage('${fullImagePath}')">
        <div class="post-content">${processedContent}</div>
        ${post.replies.length > 0 ? `<p class="reply-toggle">Εμφάνιση νήματος (${post.replies.length})</p>` : ''}
        <div class="reply-container" style="display: none;"></div>
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




function openFullImage(imagePath) {
    const win = window.open(imagePath, '_blank');
    win.focus();
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
        'img/rand/5.png',
        // Add more image URLs as needed
    ];

    // Function to select a random image
    function getRandomImage() {
        const randomIndex = Math.floor(Math.random() * images.length);
        return images[randomIndex];
    }

    // Set the image source on page load
    document.getElementById('randomImage').src = getRandomImage();

    let currentThreadId = null; // Holds the ID of the current thread (post)

// Function to load the thread - show only the post and its replies
function loadThread() {
    const params = new URLSearchParams(window.location.search);
    const postId = params.get('postId');  // Get the postId from the URL
    currentThreadId = postId;

    if (!postId) {
        document.getElementById('thread-posts').innerText = "No thread selected.";
        return;
    }

    // Example data: Replace with real data fetching from the server
    const allPosts = [
        { id: 1, content: "This is the first post of the thread.", replyTo: null },
        { id: 2, content: "This is a reply to post 1.", replyTo: 1 },
        { id: 3, content: "This is another main post.", replyTo: null },
        { id: 4, content: "This is a reply to post 3.", replyTo: 3 },
        { id: 5, content: "This is a reply to post 2.", replyTo: 2 },
    ];

    const postsMap = {};
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

    const threadPostsContainer = document.getElementById('thread-posts');
    threadPostsContainer.innerHTML = '';

    // Get the main post from the thread (the post selected by postId)
    const mainPost = postsMap[postId];

    if (mainPost) {
        // Display the main post and its replies
        const collectThread = (post) => {
            // Create a new div to show the post content
            const postElement = document.createElement('div');
            postElement.className = "post";
            postElement.innerHTML = `
                <p><strong>Post ID ${post.id}:</strong> ${post.content}</p>
            `;
            threadPostsContainer.appendChild(postElement);

            // Recursively show replies
            post.replies.forEach(reply => collectThread(reply));
        };

        collectThread(mainPost);  // Start with the main post
    }
}

// Handle the submission of a new reply to the thread
async function handleReplySubmit(event) {
    event.preventDefault();

    const content = document.getElementById('content').value;

    if (!currentThreadId || !content) {
        alert('Missing required fields!');
        return;
    }

    // Simulate a new post (in a real app, send data to the server)
    const newPost = {
        id: Date.now(), // Unique ID based on timestamp
        content,
        replyTo: currentThreadId, // Link to the current thread
    };

    // Simulate saving the new post (you would make an API call here)
    console.log('New post created:', newPost);

    // Clear the form
    document.getElementById('reply-form').reset();

    // Reload the thread to show the new post (in a real app, fetch updated data)
    loadThread();
}

// Event listener for the form submission
document.addEventListener('DOMContentLoaded', () => {
    loadThread();

    const form = document.getElementById('reply-form');
    form.addEventListener('submit', handleReplySubmit);
});
